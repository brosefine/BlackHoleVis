#include <iostream>

#include <rendering/glBoilerplate.h>
#include <rendering/shader.h>
#include <rendering/quad.h>
#include <rendering/camera.h>
#include <rendering/window.h>
#include <objects/blackHole.h>
#include <gui/gui.h>

float t0 = 0.f, dt = 0.f;
Shader blackHoleShader;
BlackHole blackHole; 
Camera cam;

bool show_demo_window = true;

void reloadShaders(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		blackHoleShader.reload();
}

int main() {

	BHVWindow window(800, 600, "Black Hole Vis");
	BHVGui gui(window.getPtr());
	

	std::vector<std::string> flags {"EHSIZE", "TESTDIST"};
	blackHoleShader = Shader("blackHole.vert", "newton.frag", flags);
	//blackHoleShader.setFlag("EHSIZE", true);
	blackHoleShader.reload();
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

	while (!window.shouldClose()) {

		gui.renderStart();

		float now = glfwGetTime();
		dt = now - t0;
		t0 = now;

		reloadShaders(window.getPtr());
		cam.keyBoardInput(window.getPtr(), dt);
		cam.mouseInput(window.getPtr());

		glClearColor(0.5, 0.5, 0.5, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		blackHoleShader.use();

		if (cam.hasChanged()) {
			blackHoleShader.setUniform("cameraPos", cam.getPosition());
			blackHoleShader.setUniform("projectionViewInverse", 
				glm::inverse(cam.getProjectionMatrix((float)window.getWidth() / window.getHeight()) * cam.getViewMatrix()));
		}
		quad.draw(GL_TRIANGLES);

		gui.renderEnd();
		window.endFrame();
	}

	return 0;

}

