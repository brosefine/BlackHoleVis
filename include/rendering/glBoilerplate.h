#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace glBoilerplate {

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

GLFWwindow* init(int width, int height) {
	// GLFW
	glfwInit();
	// checks if correct OpenGL versions are present
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// core profile includes smaller set of functions, e.g. no backwards-compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* windowPtr = glfwCreateWindow(width, height, "Black Hole Vis", NULL, NULL);
	if (!windowPtr) {
		std::cout << "Window creation failed" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	glfwMakeContextCurrent(windowPtr);
	glfwSetFramebufferSizeCallback(windowPtr, framebufferResizeCallback);

	// GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return nullptr;
	}

	return windowPtr;
}

} // glBoilerplate