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
	, t0_(0.f), dt_(0.f), tPassed_(0.f)
	, vSync_(true)
	, showShaders_(false)
	, showCamera_(false)
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
		gridChange_ = false;
		joinGridThread();
		resizeGridTextures();
		initMakeGridSSBO();
		makeNewGrid_ = true;
	}

	cam_.processInput(window_.getPtr(), dt_);

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
		currentCubeMap_->bind();

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

		if (makeNewGrid_) {

			gpuMakeGrid(true);
			makeNewGrid_ = false;
		}

		fbo = gpuGrid_;
		break;
	case KerrApp::RenderMode::INTERPOLATE:
		if (makeNewGrid_) {

			gpuMakeGrid(false);
			gpuInterpolate(true);

			makeNewGrid_ = false;
		}

		fbo = interpolatedGrid_;
		break;
	case KerrApp::RenderMode::RENDER:
		if (makeNewGrid_) {
			gpuMakeGrid(false);
			gpuInterpolate(false);

			makeNewGrid_ = false;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fboTexture_->getFboId());
		glViewport(0, 0, fboTexture_->getWidth(), fboTexture_->getHeight());
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		currentCubeMap_->bind();

		glActiveTexture(GL_TEXTURE1);
		interpolatedGrid_->bind();

		glActiveTexture(GL_TEXTURE2);
		mwPanorama_->bind();

		renderShader_->getShader()->use();
		quad_.draw(GL_TRIANGLES);

		break;
	default:
		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_.getWidth(), window_.getHeight());
	glClear(GL_COLOR_BUFFER_BIT);

	sQuadShader_->use();

	glActiveTexture(GL_TEXTURE0);
	fbo->bind();

	quad_.draw(GL_TRIANGLES);
}

void KerrApp::initShaders() {
	sQuadShader_ = std::make_shared<Shader>("squad.vs", "squad.fs");
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
		{GL_TEXTURE_WRAP_S, GL_REPEAT},
		{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
		{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
		{GL_TEXTURE_MAG_FILTER, GL_LINEAR}
	};
	interpolatedGrid_->setParam(texParameters);
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
	Grid::makeGrid(tmpGrid, properties_);
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

	glDispatchCompute(makeGridWorkGroups_.x, makeGridWorkGroups_.y, 1);
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

	glDispatchCompute(interpolateWorkGroups_.x, interpolateWorkGroups_.y, 1);
}

void KerrApp::renderGui() {

	if (showFps_)
		renderFPSWindow();

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

			// FBO and window size
			ImGui::Text("Set Offscreen Resolution");
			if (ImGui::SliderInt("* window size", &fboScale_, 1, 5))
				resizeTextures();
			if (ImGui::Checkbox("VSYNC", &vSync_))
				glfwSwapInterval((int)vSync_);

			ImGui::Checkbox("Show FPS", &showFps_);
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
	}
	ImGui::EndTabBar();
	ImGui::End();
}

void KerrApp::renderShaderTab() {
	if (ImGui::Button("Reload Shaders"))
		reloadShaders();
	ImGui::Separator();

	ImGui::Text("RenderMode");
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
		joinGridThread();
		gridThread_ = std::make_shared<std::thread>(&KerrApp::makeGrid, this);
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
	ImGui::SameLine(); ImGui::SliderInt("Print level: ", &printLvl, 0, 8);
}

void KerrApp::renderCameraTab() {

	ImGui::Text("Camera Settings");

	static bool friction = true;
	ImGui::SameLine();
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
		std::cerr << "[BHVApp] file " << ROOT_DIR "saves/kerr/" + file << " not found" << std::endl;
		return;
	}
	std::ifstream inFile(ROOT_DIR "saves/kerr/" + file);
	std::ostringstream sstr;
	sstr << inFile.rdbuf();
	std::string json = sstr.str();
	inFile.close();
	
	boost::json::value v = boost::json::parse(json);
	if (v.kind() != boost::json::kind::object) {
		std::cerr << "[BHVApp] Error parsing configuration file" << std::endl;
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