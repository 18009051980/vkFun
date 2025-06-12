#include "GlfwApp.h"


#include <iostream>

bool GlfwApp::tick(float delta) 
{
    int width{};
    int height{};
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    return glfwWindowShouldClose(window);
}




