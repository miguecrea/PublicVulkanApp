#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Camera
{
public:
    Camera() = default;

    void Init(GLFWwindow* window, float fovDegrees, float aspect, float nearPlane, float farPlane);
    void Update(float deltaTime);
    void OnResize(float aspect);

    const glm::mat4& GetView()       const { return m_View; }
    const glm::mat4& GetProjection() const { return m_Proj; }
    glm::vec3        GetPosition()   const { return m_Position; }

    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
    static Camera *  s_Instance;

    GLFWwindow* m_Window = nullptr;

    float m_Fov = 45.0f;
    float m_Aspect = 1.0f;
    float m_Near = 0.1f;
    float m_Far = 100.0f;

    glm::vec3 m_Position = { 10.240f, 0.654f, 2.956f };
    float m_Yaw = 181.700f;
    float m_Pitch = 1.100f;

    glm::vec3 m_Front = { -1.0f, -1.0f, -1.0f };
    glm::vec3 m_Up = { 0.0f,  0.0f,  1.0f };
    glm::vec3 m_Right = { 1.0f,  0.0f,  0.0f };
    glm::vec3 m_WorldUp = { 0.0f,  0.0f,  1.0f };



    float m_MoveSpeed   = 5.0f;   // m/s (3x with Shift = 15 m/s, ~sprint)
    float m_LookSpeed   = 0.05f;  // degrees per mouse pixel
    float m_ScrollSpeed = 1.0f;

    bool  m_FirstMouse = true;
    float m_LastX = 0.0f;
    float m_LastY = 0.0f;
    float m_MouseDeltaX = 0.0f;
    float m_MouseDeltaY = 0.0f;
    float m_ScrollDelta = 0.0f;

    glm::mat4 m_View = glm::mat4(1.0f);
    glm::mat4 m_Proj = glm::mat4(1.0f);

    void UpdateVectors();
    void UpdateMatrices();
    void ProcessKeyboard(float deltaTime);
    void ProcessMouse();
};