#include "Camera.h"
#include <algorithm>

Camera * Camera::s_Instance = nullptr;

void Camera::Init(GLFWwindow* window, float fovDegrees, float aspect, float nearPlane, float farPlane)
{
    s_Instance = this;
    m_Window = window;
    m_Fov = fovDegrees;
    m_Aspect = aspect;
    m_Near = nearPlane;
    m_Far = farPlane;

 
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    UpdateVectors();
    UpdateMatrices();
}

void Camera::Update(float deltaTime)
{
    ProcessKeyboard(deltaTime);
    ProcessMouse();
    UpdateMatrices();
}

void Camera::OnResize(float aspect)
{
    m_Aspect = aspect;
    UpdateMatrices();
}

void Camera::ProcessKeyboard(float deltaTime)
{
    float speed = m_MoveSpeed * deltaTime;

    if (glfwGetKey(m_Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 3.0f;

    if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS) m_Position += m_Front * speed;
    if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS) m_Position -= m_Front * speed;
    if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS) m_Position -= m_Right * speed;
    if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS) m_Position += m_Right * speed;
    if (glfwGetKey(m_Window, GLFW_KEY_E) == GLFW_PRESS) m_Position += m_WorldUp * speed;
    if (glfwGetKey(m_Window, GLFW_KEY_Q) == GLFW_PRESS) m_Position -= m_WorldUp * speed;

    // Escape toggles cursor
    static bool escWasPressed = false;
    static bool cursorVisible = false;
    bool escNow = glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escNow && !escWasPressed)
    {
        cursorVisible = !cursorVisible;
        glfwSetInputMode(m_Window, GLFW_CURSOR,
            cursorVisible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        if (!cursorVisible) m_FirstMouse = true;
    }
    escWasPressed = escNow;
}

void Camera::ProcessMouse()
{
    if (glfwGetInputMode(m_Window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
        return;

    m_Yaw += m_MouseDeltaX * m_LookSpeed;
    m_Pitch -= m_MouseDeltaY * m_LookSpeed;
    m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

    m_MouseDeltaX = 0.0f;
    m_MouseDeltaY = 0.0f;

    m_Fov -= m_ScrollDelta * m_ScrollSpeed;
    m_Fov = std::clamp(m_Fov, 10.0f, 90.0f);
    m_ScrollDelta = 0.0f;

    UpdateVectors();
}

void Camera::UpdateVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Pitch));

    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void Camera::UpdateMatrices()
{
    m_View = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    m_Proj = glm::perspective(glm::radians(m_Fov), m_Aspect, m_Near, m_Far);
    m_Proj[1][1] *= -1;
}

void Camera::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!s_Instance) return;
    float x = static_cast<float>(xpos);
    float y = static_cast<float>(ypos);

    if (s_Instance->m_FirstMouse)
    {
        s_Instance->m_LastX = x;
        s_Instance->m_LastY = y;
        s_Instance->m_FirstMouse = false;
    }

    s_Instance->m_MouseDeltaX = x - s_Instance->m_LastX;
    s_Instance->m_MouseDeltaY = y - s_Instance->m_LastY;
    s_Instance->m_LastX = x;
    s_Instance->m_LastY = y;
}

void Camera::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (s_Instance)
        s_Instance->m_ScrollDelta += static_cast<float>(yoffset);
}