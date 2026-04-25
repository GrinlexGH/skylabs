#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

class CCamera
{
public:
    enum class MoveDirection : std::int8_t
    {
        eForward,
        eBackward,
        eLeft,
        eRight
    };

    explicit CCamera(
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f),
        float yaw = YAW,
        float pitch = PITCH
    ) : m_position(position), m_front(glm::vec3(1.0f, 0.0f, 0.0f)), m_worldUp(up), m_yaw(yaw), m_pitch(pitch) {
        UpdateCameraVectors();
    }

    glm::mat4 ViewMatrix() const {
        return lookAt(m_position, m_position + m_front, m_up);
    }

    float Fov() const { return m_fov; }

    void ProcessKeyboard(const MoveDirection direction, const float deltaTime) {
        const float velocity = m_movementSpeed * deltaTime / 1000.0f;
        if (direction == MoveDirection::eForward)
            m_position += m_front * velocity;
        if (direction == MoveDirection::eBackward)
            m_position -= m_front * velocity;
        if (direction == MoveDirection::eLeft)
            m_position -= m_right * velocity;
        if (direction == MoveDirection::eRight)
            m_position += m_right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        xoffset *= m_mouseSensitivity;
        yoffset *= m_mouseSensitivity;

        m_yaw += xoffset;
        m_pitch += yoffset;

        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

        UpdateCameraVectors();
    }

    void ProcessMouseScroll(const float yoffset) {
        m_fov -= yoffset;
        if (m_fov < 1.0f)
            m_fov = 1.0f;
        if (m_fov > 120.0f)
            m_fov = 120.0f;
    }

    void MoveFaster() {
        if (m_movementSpeed <= SPEED)
            m_movementSpeed += 1.f;
    }

    void ResetSpeed() {
        m_movementSpeed = SPEED;
    }

private:
    constexpr static const float YAW = -90.0f;
    constexpr static const float PITCH = 0.0f;
    constexpr static const float SPEED = 1;
    constexpr static const float SENSITIVITY = 0.1f;
    constexpr static const float FOV = 90.0f;

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw = YAW;
    float m_pitch = PITCH;

    float m_movementSpeed = SPEED;
    float m_mouseSensitivity = SENSITIVITY;
    float m_fov = FOV;

    void UpdateCameraVectors() {
        glm::vec3 front;
        front.x = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_pitch));

        m_front = normalize(front);
        m_right = normalize(cross(m_front, m_worldUp));
        m_up = normalize(cross(m_right, m_front));
    }
};
