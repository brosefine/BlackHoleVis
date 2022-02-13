#pragma once

#include <thread>
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
#include <rendering/buffers.h>
#include <rendering/mesh.h>
#include <gui/gui.h>


class KerrApp : public GLApp{
	enum class RenderMode {
		SKY,
		COMPUTE,
		MAKEGRID,
		INTERPOLATE
	};

public:
	
	KerrApp(int width, int height);
	~KerrApp() {
		joinGridThread();
	}

private:
	void renderContent() override;
	void renderGui() override;
	void processKeyboardInput() override;

	RenderMode mode_;

	GridProperties properties_;

	SchwarzschildCamera cam_;
	std::vector<std::pair<std::string, std::shared_ptr<CubeMap>>> cubemaps_;
	std::shared_ptr<CubeMap> currentCubeMap_;
	std::shared_ptr<FBOTexture> fboTexture_;
	std::shared_ptr<FBOTexture> gpuGrid_;
	std::shared_ptr<FBOTexture> interpolatedGrid_;
	int fboScale_;

	std::shared_ptr<SSBO> testSSBO_;
	// SSBOs for makeGrid shader
	std::shared_ptr<SSBO> hashTableSSBO_;
	std::shared_ptr<SSBO> hashPosSSBO_;
	std::shared_ptr<SSBO> offsetTableSSBO_;
	std::shared_ptr<SSBO> tableSizeSSBO_;
	
	bool compute_;
	std::shared_ptr<ComputeShader> computeShader_;
	std::shared_ptr<ComputeShader> makeGridShader_;
	std::shared_ptr<ComputeShader> interpolateShader_;
	glm::ivec3 testWorkGroups_;
	glm::ivec3 makeGridWorkGroups_;
	glm::ivec3 interpolateWorkGroups_;

	Quad quad_;
	std::shared_ptr<ShaderBase> sQuadShader_;
	std::shared_ptr<ShaderBase> testShader_;

	std::shared_ptr<Grid> grid_;
	std::atomic<std::shared_ptr<Grid>> newGrid_;
	std::atomic<bool> gridChange_;
	bool makeNewGrid_;
	std::shared_ptr<std::thread> gridThread_;

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
	void resizeGridTextures();
	
	void initTestSSBO();
	void initMakeGridSSBO();
	void updateMakeGridSSBO();

	void makeGrid();
	void joinGridThread();

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