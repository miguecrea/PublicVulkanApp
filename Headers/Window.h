#pragma once

class GLFWwindow;
class Window
{
private:
    GLFWwindow * m_WindowPointer;
    void initWindow();
public:
    Window();
};