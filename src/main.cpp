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
	app->onResize();
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	static bool firstMouse = true; 
	static float lastX = 0.0f, lastY = 0.0f; 

	if (firstMouse) {
		lastX = xpos; 
		lastY = ypos;
		firstMouse = false;   
	}

	// Calculate mouse movement deltas since last callback
	float xoffset = xpos - lastX;                  
	float yoffset = lastY - ypos;                   

	// Update state for next callback iteration
	lastX = xpos;
	lastY = ypos;

	// Convert mouse movement to camera rotation
	auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
	app->onMouseMove(xpos, ypos, xoffset, yoffset);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
	app->onMousePressed(button, action, mods);
}

void mouse_wheel_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
	app->onMouseWheel(xoffset, yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
	app->onKey(key, scancode, action, mods);
}


void setWindowCallbacks(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, mouse_wheel_callback);
	glfwSetKeyCallback(window, key_callback);
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
		glfwPollEvents();

		renderer.drawFrame();
	}

	renderer.cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();
}
