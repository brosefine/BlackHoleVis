#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include <rendering/glBoilerplate.h>
#include <rendering/shader.h>
#include <rendering/quad.h>
#include <rendering/camera.h>
#include <objects/blackHole.h>

float t0 = 0.f, dt = 0.f;
Shader blackHoleShader;
BlackHole blackHole;
Camera cam;

void processKeyboardInputs(GLFWwindow* window) {

	// close window with escape
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		blackHoleShader.reload();
}

int main() {

	GLFWwindow *windowPtr = glBoilerplate::init(windowWidth, windowHeight);

	//Shader blackHoleShader("blackHole.vert", "blackHoleTest.frag");
	blackHoleShader = Shader("blackHole.vert", "newton.frag");
	Mesh quad(quadPostions, quadUVs, quadIndices);

	unsigned int blackHoleBinding = 1;
	blackHole = BlackHole({ 0.f, 0.f, 0.f }, 1.0e6, blackHoleBinding);
	blackHole.uploadData();
	blackHoleShader.use();
	blackHoleShader.setBlockBinding("blackHole", blackHoleBinding);

	std::cout << "Radius: " << blackHole.getRadius() << std::endl;

	// place camera at a distance of 10 black hole (schwarzschild-)radii
	cam = Camera({ 0.f, 0.f, -10*blackHole.getRadius() }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f });
	// set camera speed to a value appropriate to the scene's scale
	cam.setSpeed(5*blackHole.getRadius());

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

		if (cam.hasChanged()) {
			blackHoleShader.setUniform("cameraPos", cam.getPosition());
			blackHoleShader.setUniform("projectionViewInverse", 
				glm::inverse(cam.getProjectionMatrix((float)windowWidth / windowHeight) * cam.getViewMatrix()));
		}
		quad.draw(GL_TRIANGLES);
		
		glfwPollEvents();
		glfwSwapBuffers(windowPtr);
	}

	glfwTerminate();
	return 0;

}

