#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement {
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

    CCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f), float yaw = YAW, float pitch = PITCH) : m_front(glm::vec3(1.0f, 0.0f, 0.0f)), m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY), m_fov(FOV)
    {
        m_position = position;
        m_worldUp = up;
        m_yaw = yaw;
        m_pitch = pitch;
        updateCameraVectors();
    }

    CCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : m_front(glm::vec3(1.0f, 0.0f, 0.0f)), m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY), m_fov(FOV)
    {
        m_position = glm::vec3(posX, posY, posZ);
        m_worldUp = glm::vec3(upX, upY, upZ);
        m_yaw = yaw;
        m_pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(m_position, m_position + m_front, m_up);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = m_movementSpeed * deltaTime;
        if (direction == FORWARD)
            m_position += m_front * velocity;
        if (direction == BACKWARD)
            m_position -= m_front * velocity;
        if (direction == LEFT)
            m_position -= m_right * velocity;
        if (direction == RIGHT)
            m_position += m_right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= m_mouseSensitivity;
        yoffset *= m_mouseSensitivity;

        m_yaw   += xoffset;
        m_pitch += yoffset;

        if (constrainPitch) {
            m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
        }

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        m_fov -= (float)yoffset;
        if (m_fov < 1.0f)
            m_fov = 1.0f;
        if (m_fov > 45.0f)
            m_fov = 45.0f;
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_pitch));

        m_front = glm::normalize(front);
        m_right = glm::normalize(glm::cross(m_front, m_worldUp));
        m_up    = glm::normalize(glm::cross(m_right, m_front));
    }
};

extern CCamera g_camera;
