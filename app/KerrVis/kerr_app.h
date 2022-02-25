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
#include "guiElements.h"

#define MAX_STAR_LOD 6

#define COMPUTE_PERFORMANCE
#define PERF_QUERY_BUFFERS 2
#define PERF_QUERY_COUNT 2
#define MAKEGRID_QUERY 0
#define INTERPOLATE_QUERY 1


class KerrApp : public GLApp{
	enum class RenderMode {
		SKY,
		COMPUTE,
		MAKEGRID,
		INTERPOLATE,
		RENDER
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
	std::shared_ptr<CubeMap> galaxyTexture_;
	std::shared_ptr<CubeMap> starTexture_;	// for "manual" rendering as point light sources
	std::shared_ptr<CubeMap> starTexture2_;	// for default sampling at high LOD values
	std::shared_ptr<Texture2D> mwPanorama_;
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
	std::shared_ptr<BlackHoleShaderGui> renderShader_;
	glm::ivec3 testWorkGroups_;
	glm::ivec3 makeGridWorkGroups_;
	glm::ivec3 interpolateWorkGroups_;

	Quad quad_;
	std::shared_ptr<QuadShaderGui> sQuadShader_;
	std::shared_ptr<ShaderBase> testShader_;

	std::shared_ptr<Grid> grid_;
	std::atomic<std::shared_ptr<Grid>> newGrid_;
	std::atomic<bool> gridChange_;
	bool makeNewGrid_;
	std::shared_ptr<std::thread> gridThread_;

	bool aberration_;
	glm::vec3 direction_;
	float speed_;

	// timing variables for render loop
	double t0_, dt_;
	float tPassed_;

	double makeGridTime_, interpolateTime_;

	bool vSync_;
	bool modePerformance_;

	unsigned int queryIDs_[PERF_QUERY_BUFFERS][PERF_QUERY_COUNT];
	unsigned int queryBackBuffer_ = 0, queryFrontBuffer_ = 1;

	void initShaders();
	void reloadShaders();
	void initCubeMaps();
	void initTextures();
	void resizeTextures();
	void resizeGridTextures();
	void loadStarTextures();
	void loadStarTile(int level, int ti, int tj, int face, int faceSize, int tileSize, GLenum target, std::string path);

	void initTestSSBO();
	void initMakeGridSSBO();
	void updateMakeGridSSBO();

	void makeGrid();
	void joinGridThread();

	void uploadCameraVectors();

	void gpuMakeGrid(bool print);
	void gpuInterpolate(bool print);

	void renderShaderTab();
	void renderCameraTab();
	void renderSkyTab();
	void renderGridTab();
	void renderPerfWindow();

	void initPerformanceQueries();
	unsigned int getPerformanceQuery(int count);

	void dumpState(std::string const& file);
	void readState(std::string const& file);
	void printDebug();

};