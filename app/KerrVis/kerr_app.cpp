#include <kerr_app.h>
#include <helpers/RootDir.h>
#include <helpers/uboBindings.h>
#include <gui/gui_helpers.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <numeric>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <boost/json.hpp>


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>


template <typename T>
std::vector<T> readFile(std::string const& path) {

	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) {
		std::cerr << "[BHV App] Error reading file " << path << std::endl;
		return {};
	}

	auto end = file.tellg();
	file.seekg(0, std::ios::beg);
	std::size_t fileSize = end - file.tellg();

	if (fileSize == 0) return {};
	if (fileSize % sizeof(T) != 0) {
		std::cerr << "[BHV App] Error reading data from file " << path << ". Invalid file size." << std::endl;
		return {};
	}

	std::vector<T> data(fileSize/sizeof(T));

	if (!file.read((char*)data.data(), data.size()*sizeof(T))) {
		std::cerr << "[BHV App] Error reading data from file " << path << std::endl;
		return {};
	}

	return data;
}

KerrApp::KerrApp(int width, int height)
	: GLApp(width, height, "Black Hole Vis")
	, mode_(RenderMode::SKY)
	, cam_({ 0.f, 0.f, -10.f })
	, fboTexture_(std::make_shared<FBOTexture>(width, height))
	, gpuGrid_(std::make_shared<FBOTexture>(1, 1))
	, interpolatedGrid_(std::make_shared<FBOTexture>(1, 1))
	, fboScale_(1)
	, compute_(false)
	, gridChange_(false)
	, makeNewGrid_(false)
	, aberration_(false)
	, direction_(1.f, 0.f, 0.f)
	, speed_(0.5f)
	, renderEnvironment_(false)
	, t0_(0.f), dt_(0.f), tPassed_(0.f)
	, vSync_(true)
	, modePerformance_(false)
	, gridDone_(false)
{
	showGui_ = true;
	cam_.update(window_.getWidth(), window_.getHeight());
	cam_.use(window_.getWidth(), window_.getHeight(), false);
	initShaders();
	initCubeMaps();
	initTextures();
	resizeTextures();
	initTestSSBO();

	// init first grid twice because first execution
	// always fails for some reason
	GridProperties tmpProps;
	tmpProps.grid_maxLvl_ = 1;
	grid_ = std::make_shared<Grid>(tmpProps);
	resizeGridTextures();
	initMakeGridSSBO();
	makeNewGrid_ = true;
	initPerformanceQueries();
}

void KerrApp::renderContent() 
{
	double now = glfwGetTime();
	dt_ = now - t0_;
	t0_ = now;
	tPassed_ += dt_;

	if (gridChange_) {
		grid_ = newGrid_;
		newGrid_ = nullptr;
		std::cout << "Grid changed!" << std::endl;
		joinGridThread();
		resizeGridTextures();
		initMakeGridSSBO();
		gridChange_ = false;
		makeNewGrid_ = true;
		gridDone_ = true;
	}

	cam_.processInput(window_.getPtr(), dt_);

	if (renderEnvironment_) {
		currentEnvironmentScene_->render(cam_.getPositionXYZ(), dt_);
		cam_.use(window_.getWidth(), window_.getHeight(), false);
	}

	uploadCameraVectors();
	if (window_.hasChanged()) {
		resizeTextures();
		cam_.update(window_.getWidth(), window_.getHeight());
		uploadCameraVectors();

	} else if (cam_.hasChanged()) {
		cam_.update(window_.getWidth(), window_.getHeight());
		uploadCameraVectors();
	}
	
	// different paths may write to different fbos
	std::shared_ptr<FBOTexture> fbo = fboTexture_;
	switch (mode_)
	{
	case KerrApp::RenderMode::SKY:

		glBindFramebuffer(GL_FRAMEBUFFER, fboTexture_->getFboId());
		glViewport(0, 0, fboTexture_->getWidth(), fboTexture_->getHeight());
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		if (renderEnvironment_) currentEnvironmentScene_->bindEnv();
		else currentCubeMap_->bind();

		testShader_->use();
		quad_.draw(GL_TRIANGLES);
		break;

	case KerrApp::RenderMode::COMPUTE:

		computeShader_->use();
		fboTexture_->bindImageTex(0, GL_WRITE_ONLY);
		testSSBO_->bindBase(3);

		glDispatchCompute(testWorkGroups_.x, testWorkGroups_.y, 1);
		break;

	case KerrApp::RenderMode::MAKEGRID:

		if (makeNewGrid_ || modePerformance_) {

			gpuMakeGrid(true);
			std::swap(queryFrontBuffer_, queryBackBuffer_);
			makeNewGrid_ = false;
		}

		fbo = gpuGrid_;
		break;
	case KerrApp::RenderMode::INTERPOLATE:
		if (makeNewGrid_ || modePerformance_) {

			gpuMakeGrid(false);
			gpuInterpolate(true);
			std::swap(queryFrontBuffer_, queryBackBuffer_);
			makeNewGrid_ = false;
		}

		fbo = interpolatedGrid_;
		break;
	case KerrApp::RenderMode::RENDER:
		if (makeNewGrid_ || modePerformance_) {
			gpuMakeGrid(false);
			gpuInterpolate(false);
			std::swap(queryFrontBuffer_, queryBackBuffer_);

			makeNewGrid_ = false;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fboTexture_->getFboId());
		glViewport(0, 0, fboTexture_->getWidth(), fboTexture_->getHeight());
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		if (renderEnvironment_) {
			renderShader_->getShader()->setUniform("gaiaMap", false);
			currentEnvironmentScene_->bindEnv();
		}
		else {
			renderShader_->getShader()->setUniform("gaiaMap", currentCubeMap_ == galaxyTexture_);
			currentCubeMap_->bind();
		}
		glActiveTexture(GL_TEXTURE1);
		interpolatedGrid_->bind();
		glActiveTexture(GL_TEXTURE2);
		mwPanorama_->bind();
		glActiveTexture(GL_TEXTURE3);
		starTexture_->bind();
		glActiveTexture(GL_TEXTURE4);
		starTexture2_->bind();

		renderShader_->getShader()->use();
		quad_.draw(GL_TRIANGLES);

		break;
	default:
		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_.getWidth(), window_.getHeight());
	glClear(GL_COLOR_BUFFER_BIT);

	sQuadShader_->getShader()->use();

	glActiveTexture(GL_TEXTURE0);
	fbo->bind();

	quad_.draw(GL_TRIANGLES);
}

void KerrApp::initShaders() {
	sQuadShader_ = std::make_shared<QuadShaderGui>();
	testShader_ = std::make_shared<Shader>("kerr/sky.vs", "kerr/sky.fs");
	computeShader_ = std::make_shared<ComputeShader>("kerr/compute.comp");
	makeGridShader_ = std::make_shared<ComputeShader>("kerr/makeGrid.comp");
	interpolateShader_ = std::make_shared<ComputeShader>("kerr/pixInterpolation.comp");
	renderShader_ = std::make_shared<BlackHoleShaderGui>();
	reloadShaders();
}

void KerrApp::reloadShaders()
{
	computeShader_->reload();
	makeGridShader_->reload();
	interpolateShader_->reload();
	testShader_->reload();
	testShader_->setBlockBinding("camera", CAMBINDING);
	makeNewGrid_ = true;
}

void KerrApp::initCubeMaps() {

	std::vector<std::string> mwPaths{
		"milkyway2048/right.png",
		"milkyway2048/left.png",
		"milkyway2048/top.png",
		"milkyway2048/bottom.png",
		"milkyway2048/front.png",
		"milkyway2048/back.png"
	};
	cubemaps_.push_back({ "Milky Way", std::make_shared<CubeMap>(mwPaths) });

	std::vector<std::string> grid{
		"gradient/right.png",
		"gradient/left.png",
		"gradient/top.png",
		"gradient/bottom.png",
		"gradient/front.png",
		"gradient/back.png"
	};
	cubemaps_.push_back({ "Gradient Grid", std::make_shared<CubeMap>(grid) });

	std::vector<std::string> bwgrid{
		"grid/right.png",
		"grid/right.png",
		"grid/top.png",
		"grid/bottom.png",
		"grid/right.png",
		"grid/right.png"
	};
	cubemaps_.push_back({ "Grid", std::make_shared<CubeMap>(bwgrid) });

	std::vector<std::pair<GLenum, GLint>> texParametersi{
		{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
		{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
	};
	for (const auto& [name, map] : cubemaps_) {
		// configure cubemap
		map->generateMipMap();
		map->setParam(texParametersi);
		map->setParam(GL_TEXTURE_MAX_ANISOTROPY, 1.0f);
	}
	currentCubeMap_ = cubemaps_.at(0).second;
	loadStarTextures();
	cubemaps_.push_back({ "Gaia Sky" , galaxyTexture_ });

	// init scenes
	environmentScenes_.insert({ "Solar System", std::make_shared<SolarSystemScene>(2048) });
	environmentScenes_.insert({ "Checker Sphere", std::make_shared<CheckerSphereScene>(2048) });
	currentEnvironmentScene_ = environmentScenes_["Solar System"];

}

void KerrApp::initTextures() {
	mwPanorama_ = std::make_shared<Texture2D>("milkyway_eso0932a.jpg");

	std::vector<std::pair<GLenum, GLint>> texParametersi{
		{GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
		{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
	};
	mwPanorama_->generateMipMap();
	mwPanorama_->setParam(texParametersi);
	mwPanorama_->setParam(GL_TEXTURE_MAX_ANISOTROPY, 1.0f);

}

void KerrApp::loadStarTextures() {
	int textureSize = 2048;

#pragma region galaxy texture
	galaxyTexture_ = std::make_shared<CubeMap>(textureSize, textureSize);
	galaxyTexture_->bind();
	std::vector<std::pair<GLenum, GLint>> texParametersi{
		{GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
		{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
		{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
		{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
		{GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE}
	};
	galaxyTexture_->setParam(texParametersi);
	glTextureStorage2D(galaxyTexture_->getTexId(), 12, GL_RGB9_E5,
		textureSize, textureSize);
#pragma endregion

#pragma region star texture 1
	starTexture_ = std::make_shared<CubeMap>(textureSize, textureSize);
	starTexture_->bind();
	texParametersi = {
		{GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST},
		{GL_TEXTURE_MAG_FILTER, GL_NEAREST},
		{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
		{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
		{GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE}
	};
	starTexture_->setParam(texParametersi);
	glTextureStorage2D(starTexture_->getTexId(), 12, GL_RGB9_E5,
		textureSize, textureSize);
	starTexture_->unbind();
#pragma endregion

#pragma region star texture 2
	starTexture2_ = std::make_shared<CubeMap>(textureSize, textureSize);
	starTexture2_->bind();
	texParametersi = {
		{GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
		{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
		{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
		{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
		{GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE}
	};

	starTexture2_->setParam(texParametersi);
	starTexture2_->setParam(GL_TEXTURE_MAX_ANISOTROPY, 4.0f);
	// star texture 2 is used for higher LOD levels (> max_star_lod)
	// and therefore smaller
	int textureSize2 = textureSize / (1 << (MAX_STAR_LOD + 1));
	glTextureStorage2D(starTexture2_->getTexId(), 11 - MAX_STAR_LOD, GL_RGB9_E5,
		textureSize2, textureSize2);
	starTexture2_->unbind();
#pragma endregion

	std::string baseDir = TEX_DIR"ebruneton/gaia_sky_map/";
	std::vector<std::string> faces{
		"pos-x", "neg-x",
		"pos-y", "neg-y",
		"pos-z", "neg-z"
	};

	Timer tim;
	tim.start("Created star textures in ");
	// loop over the mipmap levels
	// loops up until level 4 because levels 4 and higher
	// are stored in a single file
	for (int level = 0; level <= 4; ++level) {
		// loop over cubemap faces
		for (int face = 0; face < 6; ++face) {
			// calculate size of texture at this level
			int faceSize = textureSize / (1 << level);
			int tileSize = std::min(256, faceSize);
			// if size is larger than maximum tile size
			int numTiles = faceSize / tileSize;

			// iterate over all tiles
			for (int tj = 0; tj < numTiles; ++tj) {
				for (int ti = 0; ti < numTiles; ++ti) {
					GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
					std::string path = std::format("{}{}-{}-{}-{}.dat",
						baseDir, faces.at(face), level, ti, tj);
					loadStarTile(level, ti, tj, face, faceSize, tileSize, target, path);
					//std::cout << "Loaded " << path << std::endl;
				}
			}
		}
	}
	tim.end();
	tim.printLast();
}

void KerrApp::loadStarTile(int level, int ti, int tj, int face, int faceSize, int tileSize, GLenum target, std::string path) {
	std::vector<unsigned int> tileData = readFile<unsigned int>(path);
	int start = 0;
	int currentLevel = level;
	// loop is only necessary because multiple levels are stored in level-4 tiles
	while (start < tileData.size()) {
		glTextureSubImage3D(galaxyTexture_->getTexId(), currentLevel, ti * tileSize, tj * tileSize, face, tileSize, tileSize, 1,
			GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV, &tileData.data()[start]);
		start += tileSize * tileSize;
		if (currentLevel <= MAX_STAR_LOD) {
			glTextureSubImage3D(starTexture_->getTexId(), currentLevel, ti * tileSize, tj * tileSize, face, tileSize, tileSize, 1,
				GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV, &tileData.data()[start]);
		}
		else {
			glTextureSubImage3D(starTexture2_->getTexId(), currentLevel - (MAX_STAR_LOD + 1), ti * tileSize, tj * tileSize, face, tileSize, tileSize, 1,
				GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV, &tileData.data()[start]);
		}
		start += tileSize * tileSize;
		currentLevel++;
		tileSize /= 2;
	}
}

void KerrApp::resizeTextures() {
	glm::vec2 dim{ window_.getWidth(), window_.getHeight() };
	fboTexture_->resize(fboScale_ * dim.x, fboScale_ * dim.y);

	glGetProgramiv(computeShader_->getID(), GL_COMPUTE_WORK_GROUP_SIZE, glm::value_ptr(testWorkGroups_));
	testWorkGroups_.x = std::ceil(fboTexture_->getWidth() / (float)testWorkGroups_.x);
	testWorkGroups_.y = std::ceil(fboTexture_->getHeight() / (float)testWorkGroups_.y);
}

void KerrApp::resizeGridTextures(){
	gpuGrid_->resize(grid_->M_, grid_->N_);
	interpolatedGrid_->resize(grid_->M_, grid_->N_);

	glGetProgramiv(makeGridShader_->getID(), GL_COMPUTE_WORK_GROUP_SIZE, glm::value_ptr(makeGridWorkGroups_));
	makeGridWorkGroups_.x = std::ceil(gpuGrid_->getWidth() / (float)makeGridWorkGroups_.x);
	makeGridWorkGroups_.y = std::ceil(gpuGrid_->getHeight() / (float)makeGridWorkGroups_.y);
	glGetProgramiv(interpolateShader_->getID(), GL_COMPUTE_WORK_GROUP_SIZE, glm::value_ptr(interpolateWorkGroups_));
	interpolateWorkGroups_.x = std::ceil(interpolatedGrid_->getWidth() / (float)interpolateWorkGroups_.x);
	interpolateWorkGroups_.y = std::ceil(interpolatedGrid_->getHeight() / (float)interpolateWorkGroups_.y);
	
	std::vector<std::pair<GLenum, GLint>> texParameters{
		{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
		{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE}
	};
	interpolatedGrid_->setParam(texParameters);
	gpuGrid_->setParam(texParameters);
	gpuGrid_->setParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gpuGrid_->setParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void KerrApp::initTestSSBO() {
	std::vector<float> data{ 1.0f, 0.5f, 0.8f, 0.9f };
	testSSBO_ = std::make_shared<SSBO>(sizeof(float) * data.size(), data.data());
	//testSSBO_->subData(0, sizeof(int), &i1);
	//testSSBO_->subData(sizeof(int), sizeof(int), &i2);
	//testSSBO_->subData(2*sizeof(int), sizeof(float) * data.size(), data.data());

}

void KerrApp::initMakeGridSSBO(){
	// Table sizes
	std::vector<int> tableSizes{grid_->hasher.hashTableWidth, grid_->hasher.offsetTableWidth};
	tableSizeSSBO_ = std::make_shared<SSBO>(sizeof(int)*tableSizes.size(), tableSizes.data());

	// Hash Table
	std::vector<float> &hashTable = grid_->hasher.hashTable;
	hashTableSSBO_ = std::make_shared<SSBO>(sizeof(float)*hashTable.size(), hashTable.data());

	// Hash Pos Tag Table
	std::vector<int>& hashPosTag = grid_->hasher.hashPosTag;
	hashPosSSBO_ = std::make_shared<SSBO>(sizeof(int) * hashPosTag.size(), hashPosTag.data());

	// Offset Table
	std::vector<int>& offsetTable = grid_->hasher.offsetTable;
	offsetTableSSBO_ = std::make_shared<SSBO>(sizeof(int) * offsetTable.size(), offsetTable.data());

	std::cout << "SSBO sizes: " <<
		"hashTable " << sizeof(float) * hashTable.size() / 1000 << " KB, " <<
		"hashPosSSBO " << sizeof(int) * hashPosTag.size() / 1000 << " KB, " <<
		"offsetTableSSBO " << sizeof(int) * offsetTable.size() / 1000 << " KB" << std::endl;
}

void KerrApp::makeGrid() {

	Grid::saveToFile(grid_);
	std::shared_ptr<Grid> tmpGrid = std::make_shared<Grid>();
	Timer tim;
	tim.start("Grid Computation");
	Grid::makeGrid(tmpGrid, properties_);
	tim.end();
	tim.printLast();
	newGrid_ = tmpGrid;
	gridChange_ = true;
}

void KerrApp::joinGridThread() {
	if (gridThread_) gridThread_->join();
	gridThread_ = nullptr;
}

void KerrApp::uploadCameraVectors() {
	glm::mat3 baseVectors = cam_.getBase3();
	switch (mode_) {
	case KerrApp::RenderMode::SKY:
		testShader_->setUniform("cam_right", baseVectors[0]);
		testShader_->setUniform("cam_up", baseVectors[1]);
		testShader_->setUniform("cam_front", baseVectors[2]);
		break;
	case KerrApp::RenderMode::RENDER:

			renderShader_->getShader()->setUniform("cam_tau", (glm::vec3(0.f)));
			renderShader_->getShader()->setUniform("cam_right", baseVectors[0]);
			renderShader_->getShader()->setUniform("cam_up", baseVectors[1]);
			renderShader_->getShader()->setUniform("cam_front", baseVectors[2]);
		if (!aberration_) {
			renderShader_->getShader()->setUniform("boosted_tau", (glm::vec3(0.f)));
			renderShader_->getShader()->setUniform("boosted_right", glm::vec3(1.f, 0.f, 0.f));
			renderShader_->getShader()->setUniform("boosted_up", glm::vec3(0.f, 0.f, 1.f));
			renderShader_->getShader()->setUniform("boosted_front", glm::vec3(0.f, 1.f, 0.f));
			return;
		}
		
		glm::mat4 e_static(
			glm::vec4(1.f, 0.f, 0.f, 0.f),	// tau
			glm::vec4(0.f, 1.f, 0.f, 0.f),	// right
			glm::vec4(0.f, 0.f, 0.f, 1.f),	// up
			glm::vec4(0.f, 0.f, 1.f, 0.f)	// front
		);
		glm::mat4 lorentz = cam_.getBoostFromVel(glm::normalize(direction_), speed_);
		glm::vec4 e_tau = e_static * lorentz[0];
		glm::vec4 e_right = e_static * lorentz[1];
		glm::vec4 e_up = e_static * lorentz[2];
		glm::vec4 e_front = e_static * lorentz[3];
		renderShader_->getShader()->setUniform("boosted_tau", (glm::vec3(e_tau.y, e_tau.z, e_tau.w)));
		renderShader_->getShader()->setUniform("boosted_right", (glm::vec3(e_right.y, e_right.z, e_right.w)));
		renderShader_->getShader()->setUniform("boosted_up", (glm::vec3(e_up.y, e_up.z, e_up.w)));
		renderShader_->getShader()->setUniform("boosted_front", (glm::vec3(e_front.y, e_front.z, e_front.w)));

		break;
	default:
		break;
	}

}

void KerrApp::gpuMakeGrid(bool print){
	makeGridShader_->use();
	gpuGrid_->bindImageTex(0, GL_WRITE_ONLY);
	hashTableSSBO_->bindBase(1);
	hashPosSSBO_->bindBase(2);
	offsetTableSSBO_->bindBase(3);
	tableSizeSSBO_->bindBase(4);

	makeGridShader_->setUniform("GM", grid_->M_);
	makeGridShader_->setUniform("GN", grid_->N_);
	makeGridShader_->setUniform("GN1", grid_->N_);
	makeGridShader_->setUniform("print", print);

#ifdef COMPUTE_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, queryIDs_[queryBackBuffer_][MAKEGRID_QUERY]);
#endif

	glDispatchCompute(makeGridWorkGroups_.x, makeGridWorkGroups_.y, 1);

#ifdef COMPUTE_PERFORMANCE
	glEndQuery(GL_TIME_ELAPSED);
	makeGridTime_ = getPerformanceQuery(MAKEGRID_QUERY) * 1e-6;
#endif
}

void KerrApp::gpuInterpolate(bool print){
	interpolateShader_->use();
	interpolatedGrid_->bindImageTex(0, GL_WRITE_ONLY);
	gpuGrid_->bindImageTex(1, GL_READ_ONLY);

	interpolateShader_->setUniform("Gr", 1);
	interpolateShader_->setUniform("GM", grid_->M_);
	interpolateShader_->setUniform("GN", grid_->N_);
	interpolateShader_->setUniform("GmaxLvl", grid_->MAXLEVEL_);
	interpolateShader_->setUniform("print", print);

#ifdef COMPUTE_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, queryIDs_[queryBackBuffer_][INTERPOLATE_QUERY]);
#endif

	glDispatchCompute(interpolateWorkGroups_.x, interpolateWorkGroups_.y, 1);

#ifdef COMPUTE_PERFORMANCE
	glEndQuery(GL_TIME_ELAPSED);
	interpolateTime_ = getPerformanceQuery(INTERPOLATE_QUERY) * 1e-6;
#endif
}

void KerrApp::renderGui() {

	if (showFps_)
		renderFPSWindow();

#ifdef COMPUTE_PERFORMANCE
	static bool showPerf_ = false;
	if (showPerf_)
		renderPerfWindow();
#endif // COMPUTE_PERFORMANCE

	ImGui::Begin("Application Options");
	if (ImGui::BeginTabBar("Options")) {
		if (ImGui::BeginTabItem("General")) {

			// Saving, Loading State
			static std::string file;
			ImGui::Text("Save / Load current state");
			ImGui::PushItemWidth(ImGui::GetFontSize() * 7);
			ImGui::SameLine();
			if (ImGui::Button("Save")) dumpState(file);
			ImGui::SameLine();
			if (ImGui::Button("Load")) readState(file);
			ImGui::PopItemWidth();

			ImGui::Text("Set Window Size");
			std::vector<int> dim {window_.getWidth(), window_.getHeight()};
			if (ImGui::InputInt2("window size", dim.data())) {
				window_.setWidth(dim.at(0));
				window_.setHeight(dim.at(1));
			}
			if (ImGui::Button("2:1 aspect ratio"))
				window_.setHeight(window_.getWidth() * 0.5);

			// FBO and window size
			ImGui::Text("Set Offscreen Resolution");
			if (ImGui::SliderInt("* window size", &fboScale_, 1, 5))
				resizeTextures();
			if (ImGui::Checkbox("VSYNC", &vSync_))
				glfwSwapInterval((int)vSync_);

			ImGui::Checkbox("Show FPS", &showFps_);
#ifdef COMPUTE_PERFORMANCE
			ImGui::Checkbox("Show Compute Performance", &showPerf_);
#endif // COMPUTE_PERFORMANCE
			ImGui::Spacing();
			if (ImGui::Button("Debug Print"))
				printDebug();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Shader Settings")) {
			renderShaderTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Camera Settings")) {
			renderCameraTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Grid Settings")) {
			renderGridTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Sky Settings")) {
			renderSkyTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Scene Settings")) {
			renderSceneTab();
			ImGui::EndTabItem();
		}
	}
	ImGui::EndTabBar();
	ImGui::End();
}

void KerrApp::renderShaderTab() {
	if (ImGui::Button("Reload Shaders"))
		reloadShaders();
	ImGui::Separator();

	static int defl_mode = 0; // 0 = nearest, 1 = linear
	ImGui::Text("Deflection Map Mode"); ImGui::SameLine();
	if (ImGui::RadioButton("GL_NEAREST", &defl_mode, 0) && interpolatedGrid_) {
		std::vector<std::pair<GLenum, GLint>> texParameters{
			{GL_TEXTURE_MIN_FILTER, GL_NEAREST},
			{GL_TEXTURE_MAG_FILTER, GL_NEAREST}
		};
		interpolatedGrid_->setParam(texParameters);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("GL_LINEAR", &defl_mode, 1) && interpolatedGrid_) {
		std::vector<std::pair<GLenum, GLint>> texParameters{
			{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
			{GL_TEXTURE_MAG_FILTER, GL_LINEAR}
		};
		interpolatedGrid_->setParam(texParameters);
	}

	ImGui::Separator();
	ImGui::Text("RenderMode");
	ImGui::Checkbox("Interpolate Grid each frame", &modePerformance_);
	static bool linear = true;
	if (ImGui::Checkbox("Linear Grid interpolation", &linear)) {
		interpolateShader_->setUniform("linear_interpolate", linear);
		makeNewGrid_ = true;
	}

	static int m = 0;
	if (ImGui::RadioButton("SKY", &m, 0)) {
		mode_ = RenderMode::SKY;
	}
	if (ImGui::RadioButton("COMPUTE", &m, 1)) {
		mode_ = RenderMode::COMPUTE;
	}
	if (ImGui::RadioButton("MAKEGRID", &m, 2)) {
		mode_ = RenderMode::MAKEGRID;
		makeNewGrid_ = true;
	}
	if (ImGui::RadioButton("INTERPOLATE GRID", &m, 3)) {
		mode_ = RenderMode::INTERPOLATE;
		makeNewGrid_ = true;
	}
	if (ImGui::RadioButton("RENDER", &m, 4)) {
		mode_ = RenderMode::RENDER;
		makeNewGrid_ = true;
	}
	
	if (mode_ == RenderMode::COMPUTE) {
		static int checkerSize = 10;
		if (ImGui::SliderInt("Checker Size", &checkerSize, 1, 100))
			computeShader_->setUniform("checkerSize", checkerSize);
	}

	if (mode_ == RenderMode::RENDER) {
		renderShader_->show();
	}

	ImGui::Separator();
	sQuadShader_->show();
}

void KerrApp::renderSkyTab() {
	ImGui::Text("Sky Settings");

	bool skyChange = false;
	ImGui::Separator();
	ImGui::Text("Select CubeMap");
	static int cubemap = 0;
	int index = 0;
	for (const auto& [name, map] : cubemaps_) {
		if (ImGui::RadioButton(name.c_str(), &cubemap, index++)) {
			currentCubeMap_ = map;
			skyChange = true;
		}
	}

	ImGui::Separator();
	ImGui::Text("Cubemap MipMap");
	static int mipMap = 0; // 0 = off, 1 = nearest, 2 = linear
	skyChange |= ImGui::RadioButton("Off", &mipMap, 0); ImGui::SameLine();
	skyChange |= ImGui::RadioButton("Nearest", &mipMap, 1); ImGui::SameLine();
	skyChange |= ImGui::RadioButton("Linear", &mipMap, 2);
	if (skyChange) {
		switch (mipMap) {
		case 0:
			currentCubeMap_->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			break;
		case 1:
			currentCubeMap_->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			break;
		case 2:
			currentCubeMap_->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			break;
		default:
			break;
		}
	}

	ImGui::Separator();
	ImGui::Text("Cubemap Anisotropic Filtering");
	static float aniSample = 1.0f, maxSample = -1.f;
	if (maxSample < 0) glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxSample);
	if (skyChange || ImGui::SliderFloat("#Samples", &aniSample, 1.0f, maxSample)) {
		currentCubeMap_->setParam(GL_TEXTURE_MAX_ANISOTROPY, aniSample);
	}
}

void KerrApp::renderGridTab() {
	ImGui::Text("Grid Settings");
	ImGui::Separator();

	ImGui::Text("Grid Properties");
	imgui_helpers::sliderDouble("Black Hole Spin", properties_.blackHole_a_, 0.001, 0.999);
	imgui_helpers::sliderDouble("Cam Rad", properties_.cam_rad_, 3.0, 20.0);
	imgui_helpers::sliderDouble("Cam Theta", properties_.cam_the_, 0.0, PI);
	imgui_helpers::sliderDouble("Cam Phi", properties_.cam_phi_, 0.0, PI2);
	imgui_helpers::sliderDouble("Cam Speed", properties_.cam_vel_, 0.0, 0.9);

	ImGui::SliderInt("Grid Start Level", &properties_.grid_strtLvl_, 1, 10);
	ImGui::SliderInt("Grid Max Level", &properties_.grid_maxLvl_, properties_.grid_strtLvl_+1, 20);

	ImGui::Separator();

	if (ImGui::Button("Make Grid (Load or Compute)")) {
		makeNewGrid_ = false;
		gridDone_ = false;
		joinGridThread();
		gridThread_ = std::make_shared<std::thread>(&KerrApp::makeGrid, this);
	}
	if (gridDone_) {
		ImGui::SameLine();
		ImGui::Text("Grid Computation Finished!");
	}
	

	static int printLvl = 0;
	if (ImGui::Button("Print Grid")) {
		if (grid_) {
			std::cout << "grid dims: M=" << grid_->M_ << ", N=" << grid_->N_ << std::endl;
			grid_->printGridCam(printLvl);
		}
		else
			std::cerr << "[Kerr] can't print grid: not initialized" << std::endl;
	}
	ImGui::SameLine(); ImGui::SliderInt("Print level", &printLvl, 0, 8);

	ImGui::Separator();
}

void KerrApp::renderSceneTab()
{
	ImGui::Checkbox("Render Environment Scene", &renderEnvironment_);
	if (renderEnvironment_) {

		if (ImGui::BeginListBox("")) {

			for (auto const& [name, scene] : environmentScenes_) {
				if (ImGui::Selectable(name.c_str(), false)) {
					currentEnvironmentScene_ = scene;
				}
			}
			ImGui::EndListBox();
		}
		ImGui::Separator();
		if (currentEnvironmentScene_)
			currentEnvironmentScene_->renderGui();
	}
}

void KerrApp::renderPerfWindow()
{
	double weight = 0.05;
	static double makeGridSum = 0, makeGridWeight = 0;
	static double interpolateSum = 0, interpolateWeight = 0;

	makeGridSum = (1.0 - weight) * makeGridSum + weight * makeGridTime_;
	makeGridWeight = (1.0 - weight) * makeGridWeight + weight;
	interpolateSum = (1.0 - weight) * interpolateSum + weight * interpolateTime_;
	interpolateWeight = (1.0 - weight) * interpolateWeight + weight;

	static std::string makeGridText = "", interpolateText = "";
	if (ImGui::Button("Get last compute time")) {

		makeGridText = std::format("makeGrid took {:.3f} ms", makeGridTime_).c_str();
		interpolateText = std::format("interpolate  took {:.3f} ms", interpolateTime_).c_str();
	}
	ImGui::Text(makeGridText.c_str());
	ImGui::Text(interpolateText.c_str());

	static bool showPlot = false;
	ImGui::Checkbox("Show Plot", &showPlot);
	if (showPlot) {

		static ScrollingBuffer rdata1, rdata2;
		static float t = 0;
		t += ImGui::GetIO().DeltaTime;
		rdata1.AddPoint(t, makeGridSum / makeGridWeight);
		rdata2.AddPoint(t, interpolateSum / interpolateWeight);

		static float history = 10.0f;
		ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");
		static ImPlotAxisFlags flags = ImPlotAxisFlags_AutoFit;

		ImGui::BulletText("Make Grid");

		ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
		//ImPlot::SetNextPlotLimitsY(0, 0.1, ImGuiCond_Always);
		if (ImPlot::BeginPlot("##GridRolling1", NULL, NULL, ImVec2(-1, 150), 0, flags, flags)) {
			ImPlot::PlotLine("dt", &rdata1.Data[0].x, &rdata1.Data[0].y, rdata1.Data.size(), 0, 2 * sizeof(float));
			ImPlot::EndPlot();
		}

		ImGui::BulletText("Interpolate");

		ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
		//ImPlot::SetNextPlotLimitsY(0, 200, ImGuiCond_Always);
		if (ImPlot::BeginPlot("##GridRolling2", NULL, NULL, ImVec2(-1, 150), 0, flags, flags)) {
			ImPlot::PlotLine("FPS", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 2 * sizeof(float));
			ImPlot::EndPlot();
		}
	}
}

void KerrApp::renderCameraTab() {

	ImGui::Text("Camera Settings");
	ImGui::Checkbox("Aberration", &aberration_);
	ImGui::SliderFloat3("Direction", glm::value_ptr(direction_), -1.f, 1.0f);
	ImGui::SliderFloat("Speed", &speed_, 0.f, 0.999f);

	ImGui::Separator();

	static bool friction = true;
	if (ImGui::Checkbox("Friction", &friction))
		cam_.setFriction(friction);

	ImGui::Separator();
	ImGui::Text("Camera Position");

	glm::vec3 camPos = cam_.getPositionXYZ();
	ImGui::PushItemWidth(ImGui::GetFontSize() * 7);
	bool xChange = ImGui::InputFloat("X", &camPos.x, 0.1, 1); ImGui::SameLine();
	bool yChange = ImGui::InputFloat("Y", &camPos.y, 0.1, 1); ImGui::SameLine();
	bool zChange = ImGui::InputFloat("Z", &camPos.z, 0.1, 1);
	ImGui::PopItemWidth();

	if (ImGui::Button("Point to Black Hole"))
		cam_.setViewDirXYZ(-camPos);

	if (xChange || yChange || zChange)
		cam_.setPosXYZ(camPos);
}


void KerrApp::initPerformanceQueries()
{
	glGenQueries(PERF_QUERY_COUNT, queryIDs_[queryBackBuffer_]);
	glGenQueries(PERF_QUERY_COUNT, queryIDs_[queryFrontBuffer_]);
	//glQueryCounter(queryIDs_[queryFrontBuffer_][1], GL_TIMESTAMP);
}

unsigned int KerrApp::getPerformanceQuery(int count)
{
	if (count >= PERF_QUERY_COUNT) return 0;
	GLuint64 result;
	glGetQueryObjectui64v(queryIDs_[queryFrontBuffer_][count], GL_QUERY_RESULT, &result);
	return result;
}

void KerrApp::dumpState(std::string const& file) {
	boost::json::object configuration;

	configuration["blackHole_a"] = properties_.blackHole_a_;
	configuration["cam_rad"] = properties_.cam_rad_;
	configuration["cam_the"] = properties_.cam_the_;
	configuration["cam_phi"] = properties_.cam_phi_;
	configuration["cam_vel"] = properties_.cam_vel_;
	configuration["grid_strtLvl"] = properties_.grid_strtLvl_;
	configuration["grid_maxLvl"] = properties_.grid_maxLvl_;
	
	std::string json = boost::json::serialize(configuration);
	std::ofstream outFile(ROOT_DIR "saves/kerr/" + file);
	outFile << json;
	outFile.close();
	return;
}

void KerrApp::readState(std::string const& file) {
	if (!std::filesystem::exists(ROOT_DIR "saves/kerr/" + file)) {
		std::cerr << "[KerrApp] file " << ROOT_DIR "saves/kerr/" + file << " not found" << std::endl;
		return;
	}
	std::ifstream inFile(ROOT_DIR "saves/kerr/" + file);
	std::ostringstream sstr;
	sstr << inFile.rdbuf();
	std::string json = sstr.str();
	inFile.close();
	
	boost::json::value v = boost::json::parse(json);
	if (v.kind() != boost::json::kind::object) {
		std::cerr << "[KerrApp] Error parsing configuration file" << std::endl;
		return;
	}

	boost::json::object configuration = v.get_object();
	jhelper::getValue(configuration, "blackHole_a", properties_.blackHole_a_);
	jhelper::getValue(configuration, "cam_rad", properties_.cam_rad_);
	jhelper::getValue(configuration, "cam_the", properties_.cam_the_);
	jhelper::getValue(configuration, "cam_phi", properties_.cam_phi_);
	jhelper::getValue(configuration, "cam_vel", properties_.cam_vel_);
	jhelper::getValue(configuration, "grid_strtLvl", properties_.grid_strtLvl_);
	jhelper::getValue(configuration, "grid_maxLvl", properties_.grid_maxLvl_);

	return;
}

void KerrApp::processKeyboardInput() {

	auto win = window_.getPtr();
	if (glfwGetKey(win, GLFW_KEY_G) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		showGui_ = true;
	} else if (glfwGetKey(win, GLFW_KEY_H) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		showGui_ = false;
	}
}

void KerrApp::printDebug() {
	std::cout << "Nothing to do here :)" << std::endl;


}