

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

#include "VulkanRenderer.h"

GLFWwindow* window;
VulkanRenderer renderer;

void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600)
{
	glfwInit();
	//set glfw to not use OpenGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//set to not resizable
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main()
{
	initWindow("Hello Triangle");

	if (renderer.init(window) == EXIT_FAILURE) { return EXIT_FAILURE; };

	//event loop until user closes window
	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();

	}

	renderer.cleanup();

	glfwDestroyWindow(window);
}
