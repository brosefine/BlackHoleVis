#pragma once

#include <iostream>

#include <app/app.h>
#include <helpers/uboBindings.h>
#include <helpers/json_helper.h>

#include <rendering/shader.h>
#include <rendering/schwarzschildCamera.h>
#include <rendering/window.h>
#include <rendering/texture.h>
#include <rendering/mesh.h>
#include <rendering/bloom.h>
#include <objects/blackHole.h>
#include <objects/accretionDisk.h>
#include <gui/gui.h>


class BHVApp : public GLApp{
public:
	
	BHVApp(int width, int height);


private:
	void renderContent() override;
	void renderGui() override;
	void processKeyboardInput() override;

	SchwarzschildCamera cam_;
	std::shared_ptr<CubeMap> panoramaGrid_;
	std::shared_ptr<Texture2D> deflectionTexture_;
	std::shared_ptr<Texture2D> invRadiusTexture_;
	std::shared_ptr<FBOTexture> fboTexture_;
	int fboScale_;
	
	Quad quad_;
	Shader sQuadShader_;
	std::shared_ptr<ShaderBase> brunetonDiscShader_;
	std::shared_ptr<ShaderBase> muellerDiscShader_;
	std::shared_ptr<ShaderBase> brunetonDeflectionShader_;
	std::shared_ptr<ShaderBase> starlessDeflectionShader_;
	bool showBruneton_;
	bool compareDeflection_;

	glm::vec2 discSize_;
	glm::vec2 fovXY_;

	// timing variables for render loop
	double t0_, dt_;
	float tPassed_;

	bool vSync_;

	// GUI flags
	bool showShaders_;
	bool showCamera_;

	void initShaders();
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