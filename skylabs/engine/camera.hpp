#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace sk {
class Camera {
public:
    enum class MoveDirection : std::int8_t { eForward, eBackward, eLeft, eRight };

    explicit Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = kYaw, float pitch = kPitch)
        : m_position(position),
          m_front(glm::vec3(1.0f, 0.0f, 0.0f)),
          m_worldUp(up),
          m_yaw(yaw),
          m_pitch(pitch) {
        UpdateCameraVectors();
    }

    glm::mat4 ViewMatrix() const { return glm::lookAt(m_position, m_position + m_front, m_up); }

    float Fov() const { return m_fov; }

    void ProcessKeyboard(const MoveDirection direction, const float deltaTime) {
        const float velocity = m_movementSpeed * deltaTime / 1000.0f;
        if (direction == MoveDirection::eForward) m_position += m_front * velocity;
        if (direction == MoveDirection::eBackward) m_position -= m_front * velocity;
        if (direction == MoveDirection::eLeft) m_position -= m_right * velocity;
        if (direction == MoveDirection::eRight) m_position += m_right * velocity;
    }

    void ProcessMouseMovement(float xOffset, float yOffset) {
        xOffset *= m_mouseSensitivity;
        yOffset *= m_mouseSensitivity;

        m_yaw += xOffset;
        m_pitch += yOffset;

        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

        UpdateCameraVectors();
    }

    void ProcessMouseScroll(const float yOffset) {
        m_fov -= yOffset;
        if (m_fov < 1.0f) m_fov = 1.0f;
        if (m_fov > 120.0f) m_fov = 120.0f;
    }

    void MoveFaster() {
        if (m_movementSpeed <= kSpeed) m_movementSpeed += 1.f;
    }

    void ResetSpeed() { m_movementSpeed = kSpeed; }

private:
    constexpr static float kYaw = -90.0f;
    constexpr static float kPitch = 0.0f;
    constexpr static float kSpeed = 1;
    constexpr static float kSensitivity = 0.1f;
    constexpr static float kFov = 90.0f;

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw = kYaw;
    float m_pitch = kPitch;

    float m_movementSpeed = kSpeed;
    float m_mouseSensitivity = kSensitivity;
    float m_fov = kFov;

    void UpdateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = sin(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

        m_front = glm::normalize(front);
        m_right = glm::normalize(cross(m_front, m_worldUp));
        m_up = glm::normalize(cross(m_right, m_front));
    }
};
}
