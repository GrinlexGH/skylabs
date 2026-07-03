#include <skylabs/core/launcher.hpp>
#include <skylabs/core/render/vulkan/renderer.hpp>
#include <skylabs/public/logging.hpp>

void CLauncher::Create() {
    m_sdlContext = SDL::CContext { SDL_INIT_VIDEO | SDL_INIT_AUDIO };
    m_window = SDL::CWindow { "Skylabs", 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN };

#ifdef PLATFORM_ANDROID
    SDL_SetWindowFullscreen(*m_window, true);
#endif

    m_renderer = Vulkan::CRenderer::TryToCreate(&m_window);

    auto [v, i] = GenerateDisk();
    auto oi = m_renderer->UploadMesh(v, i);
    m_towers.emplace_back(glm::vec3(-1.0f, -0.5f, -1.0f), m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 1), std::vector <SDisk> {
        { 3, false, m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 3) },
        { 2, false, m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 2) },
        { 1, false, m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 1) },
    }, 1);
    m_towers.emplace_back(glm::vec3(0.0f, -0.5f, -1.0f), m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 2), std::vector <SDisk> {}, 2);
    m_towers.emplace_back(glm::vec3(1.0f, -0.5f, -1.0f), m_renderer->UploadGameObject(oi, glm::mat4(1.0f), 3), std::vector <SDisk> {}, 3);
}

void CLauncher::UpdateVisuals(float /*deltaTime*/) {
    const float DISK_HEIGHT = 0.15f;
    const float BASE_RADIUS = 0.10f;

    for (auto& tower : m_towers) {
        glm::mat4 stemModel = glm::mat4(1.0f);
        stemModel = glm::translate(stemModel, tower.basePosition);
        stemModel = glm::scale(stemModel, glm::vec3(0.02f, 1.2f, 0.02f));
        tower.stemRenderObject.SetMatrix(stemModel);

        for (std::size_t i = 0; i < tower.disks.size(); ++i) {
            auto& disk = tower.disks[i];

            glm::vec3 diskPos = tower.basePosition + glm::vec3(0.0f, i * DISK_HEIGHT, 0.0f);
            if (disk.isselected)
                diskPos.y += 0.25f;

            float currentRadius = BASE_RADIUS * disk.size;

            glm::mat4 diskModel = glm::mat4(1.0f);
            diskModel = glm::translate(diskModel, diskPos);
            diskModel = glm::scale(diskModel, glm::vec3(currentRadius, DISK_HEIGHT, currentRadius));

            disk.renderObject.SetMatrix(diskModel);
        }
    }
}

void CLauncher::Main() {
    constexpr int TARGET_FPS = 60;
    constexpr int FRAME_DELAY = 1000 / TARGET_FPS;

    auto lastTick = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    float elapsedTime = 0.0f;

    while (!m_quit) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> diff = frameStart - lastTick;
        lastTick = frameStart;
        float deltaTimeMs = diff.count();

        ProcessEvents();

        if (!m_minimized) {
            Update(deltaTimeMs);
            UpdateVisuals(deltaTimeMs);
            Render(deltaTimeMs);
        } else {
            auto frameEnd = std::chrono::high_resolution_clock::now();
            float busyTime = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
            if (busyTime < FRAME_DELAY) {
                SDL_Delay(static_cast<Uint32>(FRAME_DELAY - busyTime));
            }
        }

        frameCount++;
        elapsedTime += deltaTimeMs;
        if (elapsedTime >= 1000.0f) {
            std::string title = "Skylabs | FPS: " + std::to_string(frameCount) + " | DT: " + std::to_string(deltaTimeMs).substr(0, 4) + "ms";
            SDL_SetWindowTitle(*m_window, title.c_str());
            Log::Debug("{}", title);
            elapsedTime -= 1000.0f;
            frameCount = 0;
        }
    }
}

void CLauncher::Destroy() {}

void CLauncher::Update(float deltaTime) {
    if (m_leftJoystick.active) {
        if (std::abs(m_leftJoystick.dirY) > 0.1f) {
            auto direction = (m_leftJoystick.dirY < 0) ? CCamera::MoveDirection::eForward : CCamera::MoveDirection::eBackward;
            m_camera.ProcessKeyboard(direction, deltaTime * std::abs(m_leftJoystick.dirY));
        }

        if (std::abs(m_leftJoystick.dirX) > 0.1f) {
            auto direction = (m_leftJoystick.dirX < 0) ? CCamera::MoveDirection::eLeft : CCamera::MoveDirection::eRight;
            m_camera.ProcessKeyboard(direction, deltaTime * std::abs(m_leftJoystick.dirX));
        }
    } else {
        const std::span keyboardState = SDL::GetKeyboardState();
        
        if (keyboardState[SDL_SCANCODE_W]) m_camera.ProcessKeyboard(CCamera::MoveDirection::eForward, deltaTime);
        if (keyboardState[SDL_SCANCODE_S]) m_camera.ProcessKeyboard(CCamera::MoveDirection::eBackward, deltaTime);
        if (keyboardState[SDL_SCANCODE_A]) m_camera.ProcessKeyboard(CCamera::MoveDirection::eLeft, deltaTime);
        if (keyboardState[SDL_SCANCODE_D]) m_camera.ProcessKeyboard(CCamera::MoveDirection::eRight, deltaTime);
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
            m_towers[m_hoveredTowerIdx].stemRenderObject.SetColor(m_towers[m_hoveredTowerIdx].baseColorId);
        }

        m_hoveredTowerIdx = closestTowerIdx;

        if (m_hoveredTowerIdx != -1) {
            m_towers[m_hoveredTowerIdx].stemRenderObject.SetColor(4);
        }
    }
}

void CLauncher::Render(float deltaTime) {
    m_renderer->Draw(m_camera.ViewMatrix(), m_camera.Fov(), deltaTime);
}

void CLauncher::ProcessEvents() {
    Uint64 flags = SDL_GetWindowFlags(*m_window);
    bool isMinimized = (flags & SDL_WINDOW_MINIMIZED) != 0;

    if (m_minimized != isMinimized) {
        m_minimized = isMinimized;
        m_renderer->m_needSwapchainRecreation = true;
    }

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT:
                m_quit = true;
                break;

            case SDL_EVENT_RENDER_DEVICE_RESET:
                m_renderer->m_needSurfaceRecreation = true;
                break;

            case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
            case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                m_renderer->m_needSwapchainRecreation = true;
                break;

            case SDL_EVENT_FINGER_DOWN:
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_FINGER_MOTION:
                HandleTouchEvent(e);
                break;

            case SDL_EVENT_KEY_DOWN:
                HandleKeyDownEvent(e.key);
                break;

            case SDL_EVENT_KEY_UP:
                HandleKeyUpEvent(e.key);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                m_camera.ProcessMouseMovement(e.motion.xrel, -e.motion.yrel);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                m_camera.ProcessMouseScroll(e.wheel.y);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                Click();
                break;

            case SDL_EVENT_TEXT_INPUT:
                HandleTextInput(e.text);
                break;
        }
    }
}

void CLauncher::Click() {
    if (m_hoveredTowerIdx == -1)
        return;
    auto [tt, dd] = m_towerDiskselecte;
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
        auto& [t, d] = m_towerDiskselecte;
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
                auto& [t, d] = m_towerDiskselecte;
                t = m_hoveredTowerIdx;
                d = mindiskIndex;
                return;
            }
        }

        m_towers[tt].disks[dd].isselected = false;
        m_towers[m_hoveredTowerIdx].disks.push_back(m_towers[tt].disks[dd]);
        m_towers[tt].disks.erase(m_towers[tt].disks.begin() + dd);
        auto& [t, d] = m_towerDiskselecte;
        t = -1;
        d = -1;
    }
}

void CLauncher::HandleTouchEvent(const SDL_Event& e) {
    if (e.type == SDL_EVENT_FINGER_DOWN) {
        // Open keyboard
        if (m_chatButton.IsInside(e.tfinger.x, e.tfinger.y)) {
            m_textInputActive = !m_textInputActive;
            m_textInputActive ? SDL_StartTextInput(*m_window) : SDL_StopTextInput(*m_window);
            return;
        }

        // Activate joystick
        if (e.tfinger.x < 0.5f && !m_leftJoystick.active) {
            m_leftJoystick.active = true;
            m_leftJoystick.fingerId = e.tfinger.fingerID;
            m_leftJoystick.centerX = e.tfinger.x;
            m_leftJoystick.centerY = e.tfinger.y;
            return;
        }
    }

    if (e.type == SDL_EVENT_FINGER_MOTION) {
        // Joystick movement
        if (m_leftJoystick.active && e.tfinger.fingerID == m_leftJoystick.fingerId) {
            float dx = e.tfinger.x - m_leftJoystick.centerX;
            float dy = e.tfinger.y - m_leftJoystick.centerY;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 0.001f) {
                float scale = (dist > m_leftJoystick.radius) ? (m_leftJoystick.radius / dist) : 1.0f;
                m_leftJoystick.dirX = (dx * scale) / m_leftJoystick.radius;
                m_leftJoystick.dirY = (dy * scale) / m_leftJoystick.radius;
            }

            return;
        }

        // If not joystick finger, then its camera movement
        const auto [width, height] = m_window.DrawableSize();
        m_camera.ProcessMouseMovement(e.tfinger.dx * width, -e.tfinger.dy * height);
    }

    if (e.type == SDL_EVENT_FINGER_UP) {
        // Disable joystick
        if (m_leftJoystick.active && e.tfinger.fingerID == m_leftJoystick.fingerId) {
            m_leftJoystick.active = false;
            m_leftJoystick.dirX = 0.0f;
            m_leftJoystick.dirY = 0.0f;
        }
    }
}

void CLauncher::HandleKeyDownEvent(const SDL_KeyboardEvent& keyEvent) {
    switch (keyEvent.key) {
        case SDLK_ESCAPE: {
            m_quit = true;
        } break;

        case SDLK_RETURN: {
            if (!m_textInputActive)
                break;
            m_textInputActive = false;
            SDL_StopTextInput(*m_window);
            Log::Debug("Keyboard Closed. Final text: {}", m_inputBuffer);
            m_inputBuffer.clear();
        } break;

        case SDLK_Z: {
            static bool mouseModeSwitch = true;
            SDL_SetWindowRelativeMouseMode(*m_window, mouseModeSwitch);
            mouseModeSwitch = !mouseModeSwitch;
        } break;

        case SDLK_F11: {
            static bool fullscreenSwitch = true;
            SDL_SetWindowFullscreen(*m_window, fullscreenSwitch);
            fullscreenSwitch = !fullscreenSwitch;
        } break;

        case SDLK_LSHIFT: {
            m_camera.MoveFaster();
        } break;
    }
}

void CLauncher::HandleKeyUpEvent(const SDL_KeyboardEvent& keyEvent) {
    switch (keyEvent.key) {
        case SDLK_LSHIFT: {
            m_camera.ResetSpeed();
        } break;
    }
}

void CLauncher::HandleTextInput(const SDL_TextInputEvent& textEvent) {
    m_inputBuffer += textEvent.text;
    Log::Info("[Input] Current buffer: {}", m_inputBuffer);
}

std::tuple<std::vector<CVertex>, std::vector<std::uint16_t>> CLauncher::GenerateDisk() {
    std::vector<CVertex> vertices;
    std::vector<std::uint16_t> indices;

    const uint32_t segments = 32;

    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * glm::pi<float>() / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);

        glm::vec3 sideNormal = glm::normalize(glm::vec3(x, 0.0f, z));
        vertices.emplace_back(glm::vec3(x, 0.0f, z), glm::vec2 {}, sideNormal);
        vertices.emplace_back(glm::vec3(x, 1.0f, z), glm::vec2 {}, sideNormal);
    }

    for (uint16_t i = 0; i < segments; ++i) {
        uint16_t b0 = i * 2;
        uint16_t t0 = b0 + 1;
        uint16_t b1 = (i + 1) * 2;
        uint16_t t1 = b1 + 1;

        indices.push_back(b0); indices.push_back(b1); indices.push_back(t1);
        indices.push_back(b0); indices.push_back(t1); indices.push_back(t0);
    }

    std::size_t topCapCenterIndex = vertices.size();
    vertices.emplace_back(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2 {}, glm::vec3(0.0f, 1.0f, 0.0f));

    std::size_t topCapEdgeStart = vertices.size();

    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * glm::pi<float>() / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);

        vertices.emplace_back(glm::vec3(x, 1.0f, z), glm::vec2 {}, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    for (uint16_t i = 0; i < segments; ++i) {
        uint16_t currentEdge = topCapEdgeStart + i;
        uint16_t nextEdge = currentEdge + 1;

        indices.push_back(topCapCenterIndex);
        indices.push_back(nextEdge);
        indices.push_back(currentEdge);
    }

    return { vertices, indices };
}
