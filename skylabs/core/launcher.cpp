#include "skylabs/core/launcher.hpp"
#include "skylabs/base/logging.hpp"
#include "skylabs/base/os.hpp"
#include "skylabs/base/sdl/filesystem.hpp"
#include "skylabs/base/sdl/sdl.hpp"
#include "skylabs/core/render/vulkan/renderer.hpp"

#ifdef PLATFORM_ANDROID
#include "skylabs/base/sdl/log_sink.hpp"
#endif

namespace sk {
void Launcher::PreCreate() {
#ifdef PLATFORM_ANDROID
    log::AddSink(std::make_unique<SDL::CLogSink>());
#else
    log::AddSink(std::make_unique<log::ConsoleSink>());
#endif
}

void Launcher::InitFilesystem() {
    m_filesystem = filesystem::Filesystem { std::make_unique<sdl::FilesystemBackend>() };

#ifdef PLATFORM_ANDROID
    m_filesystem.Mount("assets", "");
    m_filesystem.Mount("assets", "assets:/");
    m_filesystem.Mount("res", "");
#else
    m_filesystem.Mount("assets", os::JoinPath(os::GetExecutableDirectory(), "assets"));
    m_filesystem.Mount("assets", os::GetExecutableDirectory());
    m_filesystem.Mount("res", os::GetExecutableDirectory());
#endif
}

void Launcher::Create() {
    m_sdlContext = sdl::Context { SDL_INIT_VIDEO };
    m_window = sdl::Window { "Skylabs", 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN };

    InitFilesystem();

    m_osConnector = sdl::vulkan::OSConnector { *m_window };
    m_renderer.emplace(&m_window, &m_osConnector, m_filesystem);

    m_eventPump.SetEventFilter(
        [](const input::Event& event, void* userData) {
            const auto self = static_cast<Launcher*>(userData);
            if (std::holds_alternative<input::WindowExposeEvent>(event)) {
                log::Debug("Window exposed!");
                self->m_renderer->OnPossiblyWindowSizeChange();
                self->LoopIteration();
                return false;
            }
            return true;
        },
        this);

    m_eventDispatcher.sink<input::QuitEvent>().connect<&Launcher::OnQuit>(*this);
    m_eventDispatcher.sink<input::KeyEvent>().connect<&Launcher::OnKeyEvent>(*this);
    m_eventDispatcher.sink<input::DeviceResetEvent>().connect<&Launcher::OnDeviceResetEvent>(
        *this);
    m_eventDispatcher.sink<input::MouseMotionEvent>().connect<&Launcher::OnMouseMotionEvent>(
        *this);
    m_eventDispatcher.sink<input::MouseWheelEvent>().connect<&Launcher::OnMouseWheelEvent>(*this);
    m_eventDispatcher.sink<input::MouseButtonEvent>().connect<&Launcher::OnMouseButtonEvent>(
        *this);
    m_eventDispatcher.sink<input::FingerTouchEvent>().connect<&Launcher::OnFingerTouchEvent>(
        *this);
    m_eventDispatcher.sink<input::FingerMotionEvent>().connect<&Launcher::OnFingerMotionEvent>(
        *this);

    auto [v, i] = GenerateDisk();
    auto oi = m_renderer->UploadMesh(v, i);
    m_towers.emplace_back(glm::vec3(-1.0f, -0.5f, -1.0f),
                          m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 1),
                          std::vector<Disk> {
                              { 3, false, m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 3) },
                              { 2, false, m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 2) },
                              { 1, false, m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 1) },
                          },
                          1);
    m_towers.emplace_back(glm::vec3(0.0f, -0.5f, -1.0f),
                          m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 2), std::vector<Disk> { },
                          2);
    m_towers.emplace_back(glm::vec3(1.0f, -0.5f, -1.0f),
                          m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 3), std::vector<Disk> { },
                          3);
}

void Launcher::UpdateVisuals(float deltaTime) {
    static float time = 0.0f;
    time += deltaTime / 1000.0f;

    for (auto& tower : m_towers) {
        constexpr float kDiskHeight = 0.15f;
        constexpr float kBaseRadius = 0.10f;

        glm::mat4 stemModel = glm::mat4(1.0f);
        stemModel = glm::translate(stemModel, tower.basePosition);
        stemModel = glm::scale(stemModel, glm::vec3(0.02f, 1.2f, 0.02f));
        tower.stemRenderObject.SetMatrix(stemModel);

        for (std::size_t i = 0; i < tower.disks.size(); ++i) {
            auto& disk = tower.disks[i];

            glm::vec3 diskPos =
                tower.basePosition + glm::vec3(0.0f, i * kDiskHeight + 0.2f * std::sin(time), 0.0f);

            float currentRadius = kBaseRadius * disk.size;

            glm::mat4 diskModel = glm::mat4(1.0f);
            diskModel = glm::translate(diskModel, diskPos);
            diskModel = glm::scale(diskModel, glm::vec3(currentRadius, kDiskHeight, currentRadius));

            disk.renderObject.SetMatrix(diskModel);
        }
    }
}

void Launcher::LoopIteration() {
    auto frameStart = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> diff = frameStart - m_lastTick;
    m_lastTick = frameStart;
    const float deltaTimeMs = diff.count();

    if (!m_window.IsMinimized()) {
        Update(deltaTimeMs);
        UpdateVisuals(deltaTimeMs);
        Render(deltaTimeMs);
    } else {
        auto frameEnd = std::chrono::high_resolution_clock::now();
        const float busyTime = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        if (busyTime < kFrameDelay) {
            SDL_Delay(static_cast<Uint32>(kFrameDelay - busyTime));
        }
    }

    m_frameCount++;
    m_elapsedTime += deltaTimeMs;
    if (m_elapsedTime >= 1000.0f) {
        float avgFps = m_frameCount * (1000.0f / m_elapsedTime);
        float avgDt = m_elapsedTime / static_cast<float>(m_frameCount);
        std::string title = fmt::format("Skylabs | FPS: {:.0f} | DT: {:.2f}ms", avgFps, avgDt);
        SDL_SetWindowTitle(*m_window, title.c_str());
        log::Debug("{}", title);
        m_elapsedTime -= 1000.0f;
        m_frameCount = 0;
    }
}

void Launcher::Main() {
    while (!m_quit) {
        ProcessEvents();
        LoopIteration();
    }
}

void Launcher::Destroy() { }

void Launcher::Update(float deltaTime) {
    if (m_leftJoystick.active) {
        if (std::abs(m_leftJoystick.dirY) > 0.1f) {
            auto direction = (m_leftJoystick.dirY < 0) ? Camera::MoveDirection::eForward
                                                       : Camera::MoveDirection::eBackward;
            m_camera.ProcessKeyboard(direction, deltaTime * std::abs(m_leftJoystick.dirY));
        }

        if (std::abs(m_leftJoystick.dirX) > 0.1f) {
            auto direction =
                (m_leftJoystick.dirX < 0) ? Camera::MoveDirection::eLeft : Camera::MoveDirection::eRight;
            m_camera.ProcessKeyboard(direction, deltaTime * std::abs(m_leftJoystick.dirX));
        }
    } else {
        const std::span keyboardState = sdl::GetKeyboardState();

        if (keyboardState[SDL_SCANCODE_W])
            m_camera.ProcessKeyboard(Camera::MoveDirection::eForward, deltaTime);
        if (keyboardState[SDL_SCANCODE_S])
            m_camera.ProcessKeyboard(Camera::MoveDirection::eBackward, deltaTime);
        if (keyboardState[SDL_SCANCODE_A])
            m_camera.ProcessKeyboard(Camera::MoveDirection::eLeft, deltaTime);
        if (keyboardState[SDL_SCANCODE_D])
            m_camera.ProcessKeyboard(Camera::MoveDirection::eRight, deltaTime);
    }

    glm::mat4 invView = glm::inverse(m_camera.ViewMatrix());
    glm::vec3 rayOrigin = glm::vec3(invView[3]);
    glm::vec3 rayDir = glm::normalize(-glm::vec3(invView[2]));

    int closestTowerIdx = -1;
    float minT = std::numeric_limits<float>::max();

    const float hitRadius = 0.05f;
    const float towerHeight = 1.2f;

    for (std::size_t i = 0; i < m_towers.size(); ++i) {
        auto& tower = m_towers[i];

        float dx = rayOrigin.x - tower.basePosition.x;
        float dz = rayOrigin.z - tower.basePosition.z;

        float denom = rayDir.x * rayDir.x + rayDir.z * rayDir.z;
        if (denom < 0.0001f) continue;

        float t = -(dx * rayDir.x + dz * rayDir.z) / denom;
        if (t < 0.0f) continue;

        glm::vec3 hitPoint = rayOrigin + t * rayDir;

        if (hitPoint.y >= tower.basePosition.y && hitPoint.y <= tower.basePosition.y + towerHeight) {
            float distSq = (hitPoint.x - tower.basePosition.x) * (hitPoint.x - tower.basePosition.x) +
                           (hitPoint.z - tower.basePosition.z) * (hitPoint.z - tower.basePosition.z);

            if (distSq <= hitRadius * hitRadius) {
                if (t < minT) {
                    minT = t;
                    closestTowerIdx = static_cast<int>(i);
                }
            }
        }
    }

    if (closestTowerIdx != m_hoveredTowerIdx) {
        if (m_hoveredTowerIdx != -1) {
            m_towers[m_hoveredTowerIdx].stemRenderObject.SetColor(
                m_towers[m_hoveredTowerIdx].baseColorId);
        }

        m_hoveredTowerIdx = closestTowerIdx;

        if (m_hoveredTowerIdx != -1) {
            m_towers[m_hoveredTowerIdx].stemRenderObject.SetColor(4);
        }
    }
}

void Launcher::Render(float deltaTime) {
    m_renderer->Draw(m_camera.ViewMatrix(), m_camera.Fov(), deltaTime);
}

void Launcher::ProcessEvents() {
    while (auto event = m_eventPump.PollEvent()) {
        std::visit([this]<typename T>(T&& e) { m_eventDispatcher.trigger(std::forward<T>(e)); }, *event);
    }
}

void Launcher::Click() {
    if (m_hoveredTowerIdx == -1) return;
    auto& [tt, dd] = m_selectedTowerAndDisk;
    if (tt == -1) {
        if (m_towers[m_hoveredTowerIdx].disks.empty()) {
            return;
        }
        int mindiskSize = 50000;
        int mindiskIndex = 0;
        for (int j = 0; auto& i : m_towers[m_hoveredTowerIdx].disks) {
            if (i.size < mindiskSize) {
                mindiskSize = i.size;
                mindiskIndex = j;
            }
            j++;
        }

        m_towers[m_hoveredTowerIdx].disks[mindiskIndex].isselected = true;
        auto& [t, d] = m_selectedTowerAndDisk;
        t = m_hoveredTowerIdx;
        d = mindiskIndex;
    } else {
        if (m_hoveredTowerIdx == tt) {
            return;
        }
        if (!m_towers[m_hoveredTowerIdx].disks.empty()) {
            int mindiskSize = 50000;
            int mindiskIndex = 0;
            for (int j = 0; auto& i : m_towers[m_hoveredTowerIdx].disks) {
                if (i.size < mindiskSize) {
                    mindiskSize = i.size;
                    mindiskIndex = j;
                }
                j++;
            }
            if (m_towers[m_hoveredTowerIdx].disks[mindiskIndex].size < m_towers[tt].disks[dd].size) {
                if (m_towers[m_hoveredTowerIdx].disks.empty()) {
                    return;
                }
                m_towers[tt].disks[dd].isselected = false;
                m_towers[m_hoveredTowerIdx].disks[mindiskIndex].isselected = true;
                auto& [t, d] = m_selectedTowerAndDisk;
                t = m_hoveredTowerIdx;
                d = mindiskIndex;
                return;
            }
        }

        m_towers[tt].disks[dd].isselected = false;
        m_towers[m_hoveredTowerIdx].disks.push_back(m_towers[tt].disks[dd]);
        m_towers[tt].disks.erase(m_towers[tt].disks.begin() + dd);
        auto& [t, d] = m_selectedTowerAndDisk;
        t = -1;
        d = -1;
    }
}

void Launcher::OnFingerTouchEvent(const input::FingerTouchEvent e) {
    if (e.down) {
        // Open keyboard
        if (m_chatButton.IsInside(e.x, e.y)) {
            m_textInputActive = !m_textInputActive;
            m_textInputActive ? SDL_StartTextInput(*m_window) : SDL_StopTextInput(*m_window);
            return;
        }

        // Activate joystick
        if (e.x < 0.5f && !m_leftJoystick.active) {
            m_leftJoystick.active = true;
            m_leftJoystick.fingerId = e.fingerID;
            m_leftJoystick.centerX = e.x;
            m_leftJoystick.centerY = e.y;
            return;
        }
    } else {
        // Disable joystick
        if (m_leftJoystick.active && e.fingerID == m_leftJoystick.fingerId) {
            m_leftJoystick.active = false;
            m_leftJoystick.dirX = 0.0f;
            m_leftJoystick.dirY = 0.0f;
        }
    }
}

void Launcher::OnFingerMotionEvent(const input::FingerMotionEvent& e) {
    // Joystick movement
    if (m_leftJoystick.active && e.fingerID == m_leftJoystick.fingerId) {
        float dx = e.x - m_leftJoystick.centerX;
        float dy = e.y - m_leftJoystick.centerY;

        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 0.001f) {
            float scale = (dist > m_leftJoystick.kRadius) ? (m_leftJoystick.kRadius / dist) : 1.0f;
            m_leftJoystick.dirX = (dx * scale) / m_leftJoystick.kRadius;
            m_leftJoystick.dirY = (dy * scale) / m_leftJoystick.kRadius;
        }

        return;
    }

    // If not joystick finger, then its camera movement
    const auto [width, height] = m_window.DrawableSize();
    m_camera.ProcessMouseMovement(e.dx * width, -e.dy * height);
}

void Launcher::HandleKeyDownEvent(const input::Keys key) {
    switch (key) {
        case input::Keys::eEscape: {
            m_quit = true;
        } break;

        case input::Keys::eEnter: {
            if (!m_textInputActive) break;
            m_textInputActive = false;
            SDL_StopTextInput(*m_window);
            log::Debug("Keyboard Closed. Final text: {}", m_inputBuffer);
            m_inputBuffer.clear();
        } break;

        case input::Keys::eZ: {
            static bool mouseModeSwitch = true;
            SDL_SetWindowRelativeMouseMode(*m_window, mouseModeSwitch);
            mouseModeSwitch = !mouseModeSwitch;
        } break;

        case input::Keys::eF11: {
            static bool fullscreenSwitch = true;
            SDL_SetWindowFullscreen(*m_window, fullscreenSwitch);
            fullscreenSwitch = !fullscreenSwitch;
        } break;

        case input::Keys::eLeftShift: {
            m_camera.MoveFaster();
        } break;

        default:
            break;
    }
}

void Launcher::HandleKeyUpEvent(const input::Keys key) {
    switch (key) {
        case input::Keys::eLeftShift: {
            m_camera.ResetSpeed();
        } break;
        default:
            break;
    }
}

void Launcher::HandleTextInput(const SDL_TextInputEvent& textEvent) {
    m_inputBuffer += textEvent.text;
    log::Info("[Input] Current buffer: {}", m_inputBuffer);
}

std::tuple<std::vector<Vertex>, std::vector<std::uint16_t>> Launcher::GenerateDisk() {
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;

    const uint32_t segments = 32;

    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * glm::pi<float>() / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);

        glm::vec3 sideNormal = glm::normalize(glm::vec3(x, 0.0f, z));
        vertices.emplace_back(glm::vec3(x, 0.0f, z), glm::vec2 { }, sideNormal);
        vertices.emplace_back(glm::vec3(x, 1.0f, z), glm::vec2 { }, sideNormal);
    }

    for (uint16_t i = 0; i < segments; ++i) {
        uint16_t b0 = i * 2;
        uint16_t t0 = b0 + 1;
        uint16_t b1 = (i + 1) * 2;
        uint16_t t1 = b1 + 1;

        indices.push_back(b0);
        indices.push_back(b1);
        indices.push_back(t1);
        indices.push_back(b0);
        indices.push_back(t1);
        indices.push_back(t0);
    }

    std::size_t topCapCenterIndex = vertices.size();
    vertices.emplace_back(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2 { }, glm::vec3(0.0f, 1.0f, 0.0f));

    std::size_t topCapEdgeStart = vertices.size();

    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * glm::pi<float>() / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);

        vertices.emplace_back(glm::vec3(x, 1.0f, z), glm::vec2 { }, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    for (uint16_t i = 0; i < segments; ++i) {
        uint16_t currentEdge = static_cast<uint16_t>(topCapEdgeStart + i);
        uint16_t nextEdge = static_cast<uint16_t>(currentEdge + 1);

        indices.push_back(static_cast<uint16_t>(topCapCenterIndex));
        indices.push_back(nextEdge);
        indices.push_back(currentEdge);
    }

    return { vertices, indices };
}
}
