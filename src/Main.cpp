#include"../Headers/Core/VulkanApp.h"

// for exceptions
#include <iostream>
#include <stdexcept>
#include <cstdlib>


int main() 
{

    Renderer app;

    try 
    {
        app.Run();
    }
    catch (const std::exception & e) 
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}