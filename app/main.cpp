#include <iostream>

#include <helpers/uboBindings.h>
#include <rendering/shader.h>
#include <rendering/quad.h>
#include <rendering/camera.h>
#include <rendering/window.h>
#include <rendering/texture.h>
#include <objects/blackHole.h>
#include <gui/gui.h>

float t0 = 0.f, dt = 0.f;
bool show_demo_window = true;

int main() {

	BHVWindow window(800, 600, "Black Hole Vis");
	BHVGui gui(window.getPtr());

	// place camera at a distance of 10 black hole (schwarzschild-)radii
	Camera cam({ 0.f, 0.f, -10 }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f });
	// set camera speed to a value appropriate to the scene's scale
	cam.update(window.getWidth(), window.getHeight());

	Mesh quad(quadPostions, quadUVs, quadIndices);
	CubeMap sky({ "sky_right.png", "sky_left.png", "sky_top.png", "sky_bottom.png", "sky_front.png", "sky_back.png" });
	sky.bind();

	while (!window.shouldClose()) {

		gui.renderStart();

		float now = glfwGetTime();
		dt = now - t0;
		t0 = now;

		cam.keyBoardInput(window.getPtr(), dt);
		cam.mouseInput(window.getPtr());
		if(cam.hasChanged() || window.hasChanged())
			cam.update(window.getWidth(), window.getHeight());

		glClearColor(0.5, 0.5, 0.5, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		gui.getCurrentShader()->use();
		
		quad.draw(GL_TRIANGLES);

		gui.renderEnd();
		window.endFrame();
	}

	return 0;

}

