#pragma once
#include <iostream>

#include "Renderer.h"

GLFWwindow* initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600)
{
	glfwInit();
	//set glfw to not use OpenGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//set to not resizable
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	return glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) 
{
	auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

int main()
{
	GLFWwindow* window = initWindow("Hello Triangle");

	VulkanRenderer renderer;

	glfwSetWindowUserPointer(window, &renderer);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	if (renderer.init(window) == EXIT_FAILURE)
	{ 
		return EXIT_FAILURE; 
	}

	//event loop until user closes window
	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();
		renderer.drawFrame();
	}

	vkDeviceWaitIdle(renderer.logicalDevice);

	renderer.cleanup();

	glfwDestroyWindow(window);
}
