#pragma once

#include <iostream>

#include <helpers/uboBindings.h>
#include <rendering/shader.h>
#include <rendering/mesh.h>
#include <rendering/camera.h>
#include <rendering/window.h>
#include <rendering/texture.h>
#include <objects/blackHole.h>
#include <objects/accretionDisk.h>
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
	bool camOrbit_;
	float camOrbitTilt_;
	float camOrbitRad_;
	float camOrbitSpeed_;
	float camOrbitAngle_;

	Mesh quad_;
	CubeMap sky_;

	FBOTexture fboTexture_;
	Shader sQuadShader_;
	int fboScale_;

	AccDisk disk_;
	float diskRotationSpeed_;
	std::map<std::string, std::shared_ptr<Texture>> diskTextures_;
	std::string selectedTexture_;

	int selectedShader_;
	std::vector<std::shared_ptr<ShaderGui>> shaderElements_;

	// timing variables for render loop
	double t0_, dt_;
	float tPassed_;

	bool vSync_;

	// GUI flags
	bool showGui_;
	bool showShaders_;
	bool showCamera_;
	bool showDisk_;
	bool showFps_;

	std::shared_ptr<Shader> getCurrentShader();
	void initGuiElements();
	void initDiskTextures();

	void calculateCameraOrbit();

	void renderOptionsWindow();
	void renderShaderWindow();
	void renderCameraWindow();
	void renderDiskWindow();
	void renderFPSPlot();

	void dumpState();
	void readState();
	void processKeyboardInput();

};