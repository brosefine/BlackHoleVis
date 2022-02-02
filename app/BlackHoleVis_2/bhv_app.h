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
#include <cubeMapScene/SolarSystemScene.h>
#include <cubeMapScene/CheckerSphereScene.h>
#include "guiElements.h"

#define MAX_STAR_LOD 6


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
	std::shared_ptr<Texture2D> jetTexture_;
	
	std::vector<std::pair<std::string,std::shared_ptr<CubeMap>>> cubemaps_;
	std::shared_ptr<CubeMap> currentCubeMap_;
	std::shared_ptr<CubeMap> galaxyTexture_;
	std::shared_ptr<CubeMap> starTexture_;	// for "manual" rendering as point light sources
	std::shared_ptr<CubeMap> starTexture2_;	// for default sampling at high LOD values
	std::shared_ptr<CubeMap> starTextureFull_;	// for comparison to manual rendering
	std::shared_ptr<Texture2D> deflectionTexture_;
	std::shared_ptr<Texture2D> invRadiusTexture_;
	std::shared_ptr<Texture2D> blackBodyTexture_;
	std::shared_ptr<Texture3D> dopplerTexture_;
	std::shared_ptr<Texture2D> noiseTexture_;
	std::shared_ptr<FBOTexture> fboTexture_;
	int fboScale_;
	
	bool bloom_;
	Bloom bloomEffect_;
	Quad quad_;
	Shader sQuadShader_;
	std::shared_ptr<ShaderGui> shaderElement_;
	std::shared_ptr<ShaderBase> shader_;

	bool renderEnvironment_;
	std::map<std::string, std::shared_ptr<CubeMapScene>> environmentScenes_;
	std::shared_ptr<CubeMapScene> currentEnvironmentScene_;

	// timing variables for render loop
	double t0_, dt_;
	float tPassed_;

	bool vSync_;

	// GUI flags
	bool showShaders_;
	bool showCamera_;

	void initShaders();
	void initTextures();
	void initCubeMaps();
	void initScenes();
	void resizeTextures();
	void loadStarTextures();
	void loadStarTile(int level, int ti, int tj, int face, int faceSize, int tileSize, GLenum target, std::string path);

	void calculateCameraOrbit();
	void uploadBaseVectors();

	void renderShaderTab();
	void renderCameraTab();
	void renderSkyTab();
	void renderSceneTab();

	void dumpState(std::string const& file);
	void readState(std::string const& file);
	void printDebug();

};