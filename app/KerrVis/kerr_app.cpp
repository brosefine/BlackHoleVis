#include <fstream>
#include <algorithm>
#include <numeric>
#include <functional>
#include <format>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <boost/json.hpp>

#include <kerr_app.h>
#include <helpers/RootDir.h>
#include <helpers/uboBindings.h>

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
	, cam_({ 0.f, 0.f, -10.f })
	, fboTexture_(std::make_shared<FBOTexture>(width, height))
	, fboScale_(1)
	, compute_(false)
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
	resizeTextures();
}

void KerrApp::renderContent() 
{
	double now = glfwGetTime();
	dt_ = now - t0_;
	t0_ = now;
	tPassed_ += dt_;

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
	
	if (compute_) {
		computeShader_->use();
		fboTexture_->bindImageTex(0, GL_WRITE_ONLY);
		glDispatchCompute(workGroups_.x, workGroups_.y, 1);
	}
	else {

		glBindFramebuffer(GL_FRAMEBUFFER, fboTexture_->getFboId());
		glViewport(0, 0, fboTexture_->getWidth(), fboTexture_->getHeight());
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		currentCubeMap_->bind();

		testShader_->use();
		quad_.draw(GL_TRIANGLES);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_.getWidth(), window_.getHeight());
	glClear(GL_COLOR_BUFFER_BIT);

	sQuadShader_->use();

	glActiveTexture(GL_TEXTURE0);
	fboTexture_->bind();

	quad_.draw(GL_TRIANGLES);
}

void KerrApp::initShaders() {
	sQuadShader_ = std::make_shared<Shader>("squad.vs", "squad.fs");
	testShader_ = std::make_shared<Shader>("kerr/sky.vs", "kerr/sky.fs");
	computeShader_ = std::make_shared<ComputeShader>("kerr/compute.comp");

	reloadShaders();
}

void KerrApp::reloadShaders()
{
	computeShader_->reload();
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

void KerrApp::resizeTextures() {
	glm::vec2 dim{ window_.getWidth(), window_.getHeight() };
	fboTexture_->resize(fboScale_ * dim.x, fboScale_ * dim.y);

	glGetProgramiv(computeShader_->getID(), GL_COMPUTE_WORK_GROUP_SIZE, glm::value_ptr(workGroups_));
	workGroups_.x = std::ceil(fboTexture_->getWidth() / (float)workGroups_.x);
	workGroups_.y = std::ceil(fboTexture_->getHeight() / (float)workGroups_.y);
}

void KerrApp::uploadCameraVectors() {
	if (compute_) return;
	glm::mat3 baseVectors = cam_.getBase3();
	testShader_->setUniform("cam_right", baseVectors[0]);
	testShader_->setUniform("cam_up", baseVectors[1]);
	testShader_->setUniform("cam_front", baseVectors[2]);
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
	}
	ImGui::EndTabBar();
	ImGui::End();
}

void KerrApp::renderShaderTab() {
	if (ImGui::Button("Reload Shaders"))
		reloadShaders();
	ImGui::Separator();
	ImGui::Checkbox("Compute Shader", &compute_);
	
	if (compute_) {
		static int checkerSize = 10;
		if (ImGui::SliderInt("Checker Size", &checkerSize, 1, 100))
			computeShader_->setUniform("checkerSize", checkerSize);
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
	
	return;
}

void KerrApp::readState(std::string const& file) {

	
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