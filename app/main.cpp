#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include <rendering/shader.h>

int width = 800, height = 600;

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void processKeyboardInputs(GLFWwindow* window) {

	// close window with escape
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

int main() {

	// GLFW
	glfwInit();
	// checks if correct OpenGL versions are present
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// core profile includes smaller set of functions, e.g. no backwards-compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* windowPtr = glfwCreateWindow(width, height, "Black Hole Vis", NULL, NULL);
	if (!windowPtr){
		std::cout << "Window creation failed" << std::endl; 
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(windowPtr);
	glfwSetFramebufferSizeCallback(windowPtr, framebufferResizeCallback);

	// GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	Shader simpleShader("simple.vs", "simple.fs");

	while (!glfwWindowShouldClose(windowPtr)) {

		processKeyboardInputs(windowPtr);

		glClearColor(1, 0.5, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glfwPollEvents();
		glfwSwapBuffers(windowPtr);
	}

	glfwTerminate();
	return 0;

}

