#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera() = default;

    void SetPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane);
    void LookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up);

    const glm::mat4& GetView()       const { return m_View; }
    const glm::mat4& GetProjection() const { return m_Proj; }

    glm::vec3 GetPosition() const { return m_Position; }

    // Will be expanded with interactive controls (WASD + mouse) later
    void Update(float deltaTime) {}

private:
    glm::mat4 m_View = glm::mat4(1.0f);
    glm::mat4 m_Proj = glm::mat4(1.0f);
    glm::vec3 m_Position = glm::vec3(0.0f);
};
