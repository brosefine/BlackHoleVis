#pragma once

#include <iostream>

#include <helpers/uboBindings.h>
#include <rendering/shader.h>
//#include <rendering/quad.h>
#include <rendering/mesh.h>
#include <rendering/camera.h>
#include <rendering/window.h>
#include <rendering/texture.h>
#include <objects/blackHole.h>
#include <gui/gui.h>

class BHVApp {
public:
	
	BHVApp(int width, int height);

	void renderLoop();

private:

	BHVWindow window_;
	BHVGui gui_;
	Camera cam_;

	Mesh quad_;
	CubeMap sky_;

	// timing variables for render loop
	float t0_, dt_;
};