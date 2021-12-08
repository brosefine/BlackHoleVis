#pragma once

#include <iostream>

#include <app/app.h>
#include <helpers/uboBindings.h>
#include <rendering/shader.h>
#include <rendering/schwarzschildCamera.h>
#include <rendering/window.h>
#include <rendering/texture.h>
#include <rendering/mesh.h>
#include <rendering/bloom.h>
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

	SchwarzschildCamera cam_;
	bool camOrbit_;
	float camOrbitTilt_;
	float camOrbitRad_;
	float camOrbitSpeed_;
	float camOrbitAngle_;

	bool aberration_;
	bool fido_;
	bool useCustomDirection_, useLocalDirection_;
	glm::vec3 direction_;
	float speed_;

	std::shared_ptr<ParticleDiscGui> disc_;
	std::shared_ptr<Texture> diskTexture_;
	
	std::vector<std::pair<std::string,std::shared_ptr<Texture>>> cubemaps_;
	std::shared_ptr<Texture> currentCubeMap_;
	std::string deflectionPath_;
	std::shared_ptr<Texture> deflectionTexture_;
	std::string invRadiusPath_;
	std::shared_ptr<Texture> invRadiusTexture_;
	std::string blackBodyPath_;
	std::shared_ptr<Texture> blackBodyTexture_;
	std::shared_ptr<Texture> noiseTexture_;
	std::shared_ptr<FBOTexture> fboTexture_;
	int fboScale_;
	
	bool bloom_;
	Bloom bloomEffect_;
	Quad quad_;
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
	void initCubeMaps();
	void resizeTextures();

	void calculateCameraOrbit();
	void uploadBaseVectors();

	void renderShaderTab();
	void renderCameraTab();
	void renderSkyTab();
	void renderFPSWindow();

	void dumpState(std::string const& file);
	void readState(std::string const& file);
	void printDebug();

	void initFrameTimeMeasure();
	void finalizeFrameTimeMeasure();

	std::vector<float> imageData_;

};