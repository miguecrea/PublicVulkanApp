#include "Camera.h"

void Camera::SetPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane)
{
    m_Proj = glm::perspective(glm::radians(fovDegrees), aspect, nearPlane, farPlane);
    m_Proj[1][1] *= -1; // Flip Y for Vulkan
}

void Camera::LookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up)
{
    m_Position = eye;
    m_View = glm::lookAt(eye, target, up);
}
