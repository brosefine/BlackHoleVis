#pragma once

#include <iostream>

#include <helpers/uboBindings.h>
#include <rendering/shader.h>
#include <rendering/mesh.h>
#include <rendering/camera.h>
#include <rendering/window.h>
#include <rendering/texture.h>
#include <objects/blackHole.h>
#include <gui/gui.h>
#include <gui/guiElement.h>

class BHVApp {
public:
	
	BHVApp(int width, int height);

	void renderLoop();

private:

	BHVWindow window_;
	Gui gui_;
	Camera cam_;

	Mesh quad_;
	CubeMap sky_;
	Texture accretionTex_;

	int selectedShader_;
	std::vector<std::shared_ptr<ShaderGui>> shaderElements_;

	// timing variables for render loop
	float t0_, dt_, tPassed_;

	bool showShaders_;
	bool showCamera_;
	std::shared_ptr<Shader> getCurrentShader();
	void initGuiElements();
	void renderOptionsWindow();
	void renderShaderWindow();
	void renderCameraWindow();

	void dumpState();
	void readState();
};