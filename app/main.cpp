#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include <rendering/shader.h>
#include <rendering/quad.h>
#include <rendering/glBoilerplate.h>

int width = 800, height = 600;

void processKeyboardInputs(GLFWwindow* window) {

	// close window with escape
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

int main() {

	GLFWwindow *windowPtr = glBoilerplate::init(width, height);

	Shader simpleShader("simple.vs", "simple.fs");
	Mesh quad(quadPostions, quadUVs, quadIndices);

	while (!glfwWindowShouldClose(windowPtr)) {

		processKeyboardInputs(windowPtr);

		glClearColor(1, 0.5, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		simpleShader.use();
		quad.draw(GL_TRIANGLES);

		glfwPollEvents();
		glfwSwapBuffers(windowPtr);
	}

	glfwTerminate();
	return 0;

}

