#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include <rendering/shader.h>
#include <rendering/quad.h>
#include <rendering/camera.h>
#include <rendering/glBoilerplate.h>

int width = 800, height = 600;
float t0 = 0.f, dt = 0.f;

void processKeyboardInputs(GLFWwindow* window) {

	// close window with escape
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

int main() {

	GLFWwindow *windowPtr = glBoilerplate::init(width, height);

	Shader blackHoleShader("blackHole.vs", "blackHole.fs");
	Mesh quad(quadPostions, quadUVs, quadIndices);

	Camera cam({ 0.f, 0.f, 3.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, -1.f });
	glm::vec3 blackHole{0.f, 0.f, -2.f};

	while (!glfwWindowShouldClose(windowPtr)) {

		float now = glfwGetTime();
		dt = now - t0;
		t0 = now;

		processKeyboardInputs(windowPtr);
		cam.keyBoardInput(windowPtr, dt);
		cam.mouseInput(windowPtr);

		glClearColor(0.5, 0.5, 0.5, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		blackHoleShader.use();
		blackHoleShader.setUniform("cameraPos", cam.getPosition());
		blackHoleShader.setUniform("blackHolePos", blackHole);
		blackHoleShader.setUniform("projectionViewInverse", glm::inverse(cam.getProjectionMatrix((float)width / height) * cam.getViewMatrix()));
		quad.draw(GL_TRIANGLES);
		glfwPollEvents();
		glfwSwapBuffers(windowPtr);
	}

	glfwTerminate();
	return 0;

}

