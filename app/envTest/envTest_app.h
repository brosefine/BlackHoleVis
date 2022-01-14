#pragma once

#include <iostream>

#include <app/app.h>
#include <helpers/uboBindings.h>
#include <helpers/json_helper.h>

#include <rendering/shader.h>
#include <rendering/simpleCamera.h>
#include <rendering/window.h>
#include <rendering/texture.h>
#include <rendering/mesh.h>
#include <gui/gui.h>
//#include "guiElements.h"


class EnvApp : public GLApp{
public:
	
	EnvApp(int width, int height);


private:
	void renderContent() override;
	void renderGui() override;
	void processKeyboardInput() override;

	SimpleCamera cam_;
	std::shared_ptr<CubeMap> skyTexture_;
	std::shared_ptr<Texture2D> meshTexture_;
	std::shared_ptr<FBOTexture> fboTexture_;
	
	Quad quad_;
	std::shared_ptr<Mesh> mesh_;
	std::shared_ptr<ShaderBase> sQuadShader_;
	std::shared_ptr<ShaderBase> skyShader_;
	std::shared_ptr<ShaderBase> meshShader_;

	// timing variables for render loop
	double t0_, dt_;
	float tPassed_;
	float speedScale_;

	bool vSync_;

	// GUI flags
	bool showShaders_;
	bool showCamera_;

	void initShaders();
	void reloadShaders();
	void initTextures();
	void resizeTextures();
	void calcFov();

	void renderShaderTab();
	void renderCameraTab();
	void renderSkyTab();

	void dumpState(std::string const& file);
	void readState(std::string const& file);
	void printDebug();

};