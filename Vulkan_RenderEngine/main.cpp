#pragma once
#include "Renderer.h"
#include "ImGuiRenderer.h"

#include <iostream>
#include <chrono>

static ImGui_ImplVulkanH_Window g_MainWindowData;

GLFWwindow* initWindow(std::string wName = "Test Window", const int width = 1800, const int height = 1200)
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
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
		app->camera.mousePressed = true;  // Set flag to indicate left mouse button is pressed
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
		app->camera.mousePressed = false; // Clear flag when left mouse button is released
	}
}



int main()
{
	GLFWwindow* window = initWindow("Hello Triangle");
	if (!glfwVulkanSupported())
	{
		printf("GLFW: Vulkan Not Supported\n");
		return 1;
	}

	VulkanRenderer renderer;

	glfwSetWindowUserPointer(window, &renderer);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	if (renderer.init(window) == EXIT_FAILURE)
	{ 
		return EXIT_FAILURE; 
	}

	ImGuiRenderer imguiRenderer;

	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	ImGui_ImplVulkanH_Window* ImGui_window = &g_MainWindowData;
	imguiRenderer.SetupVulkanWindow(ImGui_window, renderer, w, h);

	imguiRenderer.init(renderer, ImGui_window);



	bool show_demo_window = true;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//event loop until user closes window
	while (!glfwWindowShouldClose(window)) 
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		glfwPollEvents();

		// Resize swap chain?
		int fb_width, fb_height;
		glfwGetFramebufferSize(window, &fb_width, &fb_height);
		if (fb_width > 0 && fb_height > 0 && (renderer.framebufferResized || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height))
		{
			ImGui_ImplVulkan_SetMinImageCount(renderer.swapChain.imageCount - 1);

			ImGui_ImplVulkanH_CreateOrResizeWindow(
				renderer.context.instance,
				renderer.context.physicalDevice,
				renderer.context.logicalDevice,
				&g_MainWindowData,
				renderer.context.getQueueFamilyIndices(renderer.context.physicalDevice).graphicsFamily,
				nullptr /*allocator*/,
				fb_width, fb_height,
				renderer.swapChain.imageCount - 1);

			g_MainWindowData.FrameIndex = 0;
			renderer.framebufferResized = false;
		}

		//Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
		if (!is_minimized)
		{
			ImGui_window->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
			ImGui_window->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
			ImGui_window->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
			ImGui_window->ClearValue.color.float32[3] = clear_color.w;
			imguiRenderer.FrameRender(ImGui_window, renderer, draw_data);
			imguiRenderer.FramePresent(ImGui_window, renderer);
		}
		ImGui::EndFrame();

		//renderer.drawFrame(renderer.camera.getViewMatrix(), renderer.camera.getProjectionMatrix(renderer.swapChain.extent.width / (float)renderer.swapChain.extent.height, 0.1f, 10.0f));


	}

	vkDeviceWaitIdle(renderer.context.logicalDevice);

	imguiRenderer.cleanup();
	ImGui_ImplVulkanH_DestroyWindow(renderer.context.instance, renderer.context.logicalDevice, &g_MainWindowData, nullptr /*allocator*/);
	renderer.cleanup();


	glfwDestroyWindow(window);
	glfwTerminate();

}
