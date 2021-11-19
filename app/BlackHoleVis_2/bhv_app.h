#pragma once

#include <iostream>

#include <app/app.h>
#include <helpers/uboBindings.h>
#include <rendering/shader.h>
#include <rendering/mesh.h>
#include <rendering/camera.h>
#include <rendering/window.h>
#include <rendering/texture.h>
#include <objects/blackHole.h>
#include <objects/accretionDisk.h>
#include <gui/gui.h>
#include "guiElements.h"

class BHVApp : public GLApp{
public:
	
	BHVApp(int width, int height);


private:
	void renderContent() override;
	void renderGui() override;
	void processKeyboardInput() override;

	Camera cam_;
	bool camOrbit_;
	float camOrbitTilt_;
	float camOrbitRad_;
	float camOrbitSpeed_;
	float camOrbitAngle_;

	
	CubeMap sky_;
	Texture deflectionTexture_;
	Texture invRadiusTexture_;
	FBOTexture fboTexture_;
	int fboScale_;
	
	Mesh quad_;
	Shader sQuadShader_;
	std::shared_ptr<ShaderGui> shaderElement_;
	std::shared_ptr<ShaderBase> shader_;

	// timing variables for render loop
	double t0_, dt_;
	float tPassed_;

	// performance measurement
	bool measureFrameTime_;
	float measureTime_;			// duration of measurement
	double measureStart_;		// start point of measurement
	std::string measureID_;		// user-defined name of a measurement
	int measureFrameWindow_;	// number of frames over which to average measured frame times
	std::vector<float> frameTimes_;

	bool vSync_;

	// GUI flags
	bool showShaders_;
	bool showCamera_;
	bool showFps_;

	void initShaders();
	void initTextures();
	void resizeTextures();

	void calculateCameraOrbit();

	void renderShaderTab();
	void renderCameraTab();
	void renderFPSWindow();

	void dumpState(std::string const& file);
	void readState(std::string const& file);

	void initFrameTimeMeasure();
	void finalizeFrameTimeMeasure();

};