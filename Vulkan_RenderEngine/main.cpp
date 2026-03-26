
#include <iostream>

#pragma once
#include <GLFW/include/glfw3.h>

#include "Renderer.h"



GLFWwindow* initWindow(uint32_t Width = 800, uint32_t Height = 600) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    return glfwCreateWindow(Width, Height, "Hello Triangle", nullptr, nullptr);
}

int main()
{
    GLFWwindow* window = initWindow();


    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window); 
}