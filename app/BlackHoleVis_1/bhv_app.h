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
#include <gui/guiElement.h>

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

	Mesh quad_;
	CubeMap sky_;

	FBOTexture fboTexture_;
	std::vector<std::shared_ptr<FBOTexture>> bloomTextures_;
	Shader sQuadShader_;
	int fboScale_;

	bool bloom_;
	int bloomPasses_;
	std::unique_ptr<ComputeShader> bloomShader_;

	AccDisk disk_;
	float diskRotationSpeed_;
	std::map<std::string, std::shared_ptr<Texture>> diskTextures_;
	std::string selectedTexture_;

	bool compute_;
	int selectedShader_;
	int selectedComputeShader_;
	std::vector<std::shared_ptr<ShaderGui>> shaderElements_;
	std::vector<std::shared_ptr<ShaderGui>> computeShaderElements_;

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
	bool showDisk_;
	bool showFps_;

	std::shared_ptr<ShaderBase> getCurrentShader();
	void initShaders();
	void initTextures();
	void resizeTextures();

	void calculateCameraOrbit();

	void renderShaderWindow();
	void renderCameraWindow();
	void renderDiskWindow();
	void renderFPSPlot();
	void renderComputeWindow();

	void dumpState(std::string const& file);
	void readState(std::string const& file);

	void initFrameTimeMeasure();
	void finalizeFrameTimeMeasure();

	void printComputeInfo();
	void updateComputeUniforms();
};