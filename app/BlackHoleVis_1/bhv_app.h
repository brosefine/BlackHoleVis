#pragma once

#include <iostream>

#include <app/app.h>
#include <helpers/uboBindings.h>
#include <rendering/shader.h>
#include <rendering/mesh.h>
#include <rendering/simpleCamera.h>
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

	SimpleCamera cam_;
	bool camOrbit_;
	float camOrbitTilt_;
	float camOrbitRad_;
	float camOrbitSpeed_;
	float camOrbitAngle_;

	Quad quad_;

	std::vector<std::pair<std::string, std::shared_ptr<CubeMap>>> cubemaps_;
	std::shared_ptr<CubeMap> currentCubeMap_;
	FBOTexture fboTexture_;
	std::vector<std::shared_ptr<FBOTexture>> bloomTextures_;
	Shader sQuadShader_;
	int fboScale_;

	bool bloomEffect_;
	int bloomPasses_;
	std::unique_ptr<ComputeShader> bloomShader_;

	AccDisc disc_;
	float diskRotationSpeed_;
	std::map<std::string, std::shared_ptr<Texture2D>> diskTextures_;
	std::string selectedTexture_;

	bool compute_;
	int selectedShader_;
	int selectedComputeShader_;
	std::vector<std::shared_ptr<ShaderGui>> shaderElements_;
	std::vector<std::shared_ptr<ShaderGui>> computeShaderElements_;

	// timing variables for render loop
	double t0_, dt_;
	float tPassed_;

	bool vSync_;

	// GUI flags
	bool showShaders_;
	bool showCamera_;
	bool showDisk_;

	std::shared_ptr<ShaderBase> getCurrentShader();
	void initShaders();
	void initTextures();
	void resizeTextures();

	void calculateCameraOrbit();
	void uploadCameraVectors();

	void renderShaderTab();
	void renderCameraTab();
	void renderDiskWindow();
	void renderComputeWindow();
	void renderSkyTab();


	void dumpState(std::string const& file);
	void readState(std::string const& file);

	void printComputeInfo();
	void updateComputeUniforms();
};