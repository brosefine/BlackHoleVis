#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include <rendering/glBoilerplate.h>
#include <rendering/shader.h>
#include <rendering/quad.h>
#include <rendering/camera.h>
#include <objects/blackHole.h>

float t0 = 0.f, dt = 0.f;

void processKeyboardInputs(GLFWwindow* window) {

	// close window with escape
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

int main() {

	GLFWwindow *windowPtr = glBoilerplate::init(windowWidth, windowHeight);

	Shader blackHoleShader("blackHole.vert", "blackHole.frag");
	Mesh quad(quadPostions, quadUVs, quadIndices);

	unsigned int blackHoleBinding = 1;
	BlackHole blackHole(blackHoleBinding);
	blackHole.uploadData();
	blackHoleShader.use();
	blackHoleShader.setBlockBinding("blackHole", blackHoleBinding);

	Camera cam({ 0.f, 0.f, -3.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f });

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
		blackHoleShader.setUniform("projectionViewInverse", 
			glm::inverse(cam.getProjectionMatrix((float)windowWidth / windowHeight) * cam.getViewMatrix()));
		quad.draw(GL_TRIANGLES);
		
		glfwPollEvents();
		glfwSwapBuffers(windowPtr);
	}

	glfwTerminate();
	return 0;

}

