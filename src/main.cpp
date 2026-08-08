#pragma once
#include "Renderer.h"
#include "ImGuiRenderer.h"

#include <iostream>
#include <chrono>

static ImGui_ImplVulkanH_Window g_MainWindowData;
const int WIDTH = 1800;
const int HEIGHT = 1200;

GLFWwindow* initWindow(std::string wName = "Test Window", const int width = WIDTH, const int height = HEIGHT)
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

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	// State persistence for calculating movement deltas
	// Static variables maintain state between callback invocations
	static bool firstMouse = true;          // Flag to handle initial mouse position
	static float lastX = 0.0f, lastY = 0.0f;  // Previous mouse position for delta calculation

	// Handle initial mouse position to prevent sudden camera jumps
	// First callback provides absolute position, not relative movement
	if (firstMouse) {
		lastX = xpos;               // Initialize previous position
		lastY = ypos;
		firstMouse = false;         // Disable special handling for subsequent calls
	}

	// Calculate mouse movement deltas since last callback
	// These deltas represent the amount and direction of mouse movement
	float xoffset = xpos - lastX;                   // Horizontal movement (left-right)
	float yoffset = lastY - ypos;                   // Vertical movement (inverted: screen Y increases downward, camera pitch increases upward)

	// Update state for next callback iteration
	lastX = xpos;
	lastY = ypos;

	// Convert mouse movement to camera rotation
	// Delta values drive continuous camera orientation changes
	auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
	app->camera.processMouseMovement(xoffset, yoffset);

	app->guiRenderer->handleMousePos(static_cast<float>(xpos), static_cast<float>(ypos)); // Pass mouse position to ImGui for UI interaction);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));

	if (!app->guiRenderer->isViewportHovered())
	{
		app->guiRenderer->handleMouseButton(button, action); // Pass mouse button state to ImGui for UI interaction

		// if the mouse leaves the viewport while the mouse is pressed release the camera
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		{
			app->camera.mousePressed = false;
		}
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		app->camera.mousePressed = true;  // Set flag to indicate left mouse button is pressed
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		app->camera.mousePressed = false; // Clear flag when left mouse button is released
	}
}

void mouse_wheel_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
	if (ImGui::GetIO().WantCaptureMouse)
	{
		// Pass mouse wheel input to ImGui for UI interaction
		ImGuiIO& io = ImGui::GetIO();
		io.AddMouseWheelEvent(static_cast<float>(xoffset), static_cast<float>(yoffset));
		return;
	}
}

void setWindowCallbacks(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, mouse_wheel_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

int main()
{
	GLFWwindow* window = initWindow("My first attempt at Vulkan");

	if (!glfwVulkanSupported()) {
		printf("GLFW: Vulkan Not Supported\n");
		return 1;
	}

	VulkanRenderer renderer;

	glfwSetWindowUserPointer(window, &renderer);
	setWindowCallbacks(window);

	if (renderer.init(window) == EXIT_FAILURE) { return EXIT_FAILURE; }


	//event loop until user closes window
	while (!glfwWindowShouldClose(window)) 
	{
		//static auto startTime = std::chrono::high_resolution_clock::now();

		//auto currentTime = std::chrono::high_resolution_clock::now();
		//float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		glfwPollEvents();

		renderer.drawFrame(
			renderer.camera.getViewMatrix(),
			renderer.camera.getProjectionMatrix((float)renderer.swapChain.extent.width / (float)renderer.swapChain.extent.height, 0.1f, 20.0f));
	}

	vkDeviceWaitIdle(renderer.context.logicalDevice);

	renderer.cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();
}
