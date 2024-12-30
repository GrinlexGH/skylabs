#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

constexpr float YAW         = -90.0f;
constexpr float PITCH       =  0.0f;
constexpr float SPEED       =  0.001f;
constexpr float SENSITIVITY =  0.1f;
constexpr float FOV         =  45.0f;

class CCamera
{
public:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_fov;

    explicit CCamera(
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f),
        float yaw = YAW,
        float pitch = PITCH
    ) : m_front(glm::vec3(1.0f, 0.0f, 0.0f)), m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY), m_fov(FOV)
    {
        m_position = position;
        m_worldUp = up;
        m_yaw = yaw;
        m_pitch = pitch;
        UpdateCameraVectors();
    }

    CCamera(
        float posX, float posY, float posZ,
        float upX, float upY, float upZ,
        float yaw,
        float pitch
    ) : m_front(glm::vec3(1.0f, 0.0f, 0.0f)), m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY), m_fov(FOV)
    {
        m_position = glm::vec3(posX, posY, posZ);
        m_worldUp = glm::vec3(upX, upY, upZ);
        m_yaw = yaw;
        m_pitch = pitch;
        UpdateCameraVectors();
    }

    glm::mat4 GetViewMatrix() const {
        return lookAt(m_position, m_position + m_front, m_up);
    }

    void ProcessKeyboard(const CameraMovement direction, const float deltaTime) {
        const float velocity = m_movementSpeed * deltaTime;
        if (direction == FORWARD)
            m_position += m_front * velocity;
        if (direction == BACKWARD)
            m_position -= m_front * velocity;
        if (direction == LEFT)
            m_position -= m_right * velocity;
        if (direction == RIGHT)
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

    void ProcessMouseScroll(float yoffset) {
        m_fov -= yoffset;
        if (m_fov < 1.0f)
            m_fov = 1.0f;
        if (m_fov > 120.0f)
            m_fov = 120.0f;
    }

    void MoveFaster() {
        if (m_movementSpeed <= SPEED)
            m_movementSpeed += 0.002f;
    }

    void ResetSpeed() {
        m_movementSpeed = SPEED;
    }

private:
    void UpdateCameraVectors() {
        glm::vec3 front;
        front.x = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_pitch));

        m_front = normalize(front);
        m_right = normalize(glm::cross(m_front, m_worldUp));
        m_up    = normalize(glm::cross(m_right, m_front));
    }
};

extern CCamera g_camera;
