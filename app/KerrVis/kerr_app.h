#pragma once

#include <iostream>

#include <app/app.h>
#include <helpers/uboBindings.h>
#include <helpers/json_helper.h>
#include <blacktracer/Const.h>
#include <blacktracer/Grid.h>

#include <rendering/shader.h>
#include <rendering/schwarzschildCamera.h>
#include <rendering/window.h>
#include <rendering/texture.h>
#include <rendering/mesh.h>
#include <gui/gui.h>


class KerrApp : public GLApp{

public:
	
	KerrApp(int width, int height);


private:
	void renderContent() override;
	void renderGui() override;
	void processKeyboardInput() override;

	GridProperties properties_;

	SchwarzschildCamera cam_;
	std::vector<std::pair<std::string, std::shared_ptr<CubeMap>>> cubemaps_;
	std::shared_ptr<CubeMap> currentCubeMap_;
	std::shared_ptr<FBOTexture> fboTexture_;
	int fboScale_;
	
	bool compute_;
	std::shared_ptr<ComputeShader> computeShader_;
	glm::ivec3 workGroups_;

	Quad quad_;
	std::shared_ptr<ShaderBase> sQuadShader_;
	std::shared_ptr<ShaderBase> testShader_;

	std::shared_ptr<Grid> grid_;

	// timing variables for render loop
	double t0_, dt_;
	float tPassed_;

	bool vSync_;

	// GUI flags
	bool showShaders_;
	bool showCamera_;

	void initShaders();
	void reloadShaders();
	void initCubeMaps();
	void initTextures();
	void resizeTextures();

	void uploadCameraVectors();

	void calculateGrid();

	void renderShaderTab();
	void renderCameraTab();
	void renderSkyTab();
	void renderGridTab();

	void dumpState(std::string const& file);
	void readState(std::string const& file);
	void printDebug();

};