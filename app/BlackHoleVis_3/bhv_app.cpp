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

#include <bhv_app.h>
#include <helpers/RootDir.h>
#include <helpers/uboBindings.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>


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

BHVApp::BHVApp(int width, int height)
	: GLApp(width, height, "Black Hole Vis")
	, cam_({ 0.f, 0.f, -10.f })
	, fboTexture_(std::make_shared<FBOTexture>(width, height))
	, fboScale_(1)
	, sQuadShader_("squad.vs", "squad.fs")
	, showBruneton_(true)
	, compareDeflection_(false)
	, discSize_(3.f, 12.f)
	, fovXY_(1.f, 1.f)
	, t0_(0.f), dt_(0.f), tPassed_(0.f)
	, vSync_(true)
	, showShaders_(false)
	, showCamera_(false)
{
	showGui_ = true;
	cam_.update(window_.getWidth(), window_.getHeight());
	cam_.use(window_.getWidth(), window_.getHeight(), false);
	initShaders();
	initTextures();
	calcFov();
}

void BHVApp::renderContent() 
{
	double now = glfwGetTime();
	dt_ = now - t0_;
	t0_ = now;
	tPassed_ += dt_;

	cam_.processInput(window_.getPtr(), dt_);


	if (window_.hasChanged()) {
		resizeTextures();
		cam_.update(window_.getWidth(), window_.getHeight());
		calcFov();

	} else if (cam_.hasChanged()) {
		cam_.update(window_.getWidth(), window_.getHeight());
		calcFov();
	}
	
	if (showBruneton_) {

		std::shared_ptr<ShaderBase> shader;
		if (compareDeflection_) shader = brunetonDeflectionShader_;
		else					shader = brunetonDiscShader_;

		glm::mat3 baseVectors = cam_.getBase3();
		shader->use();
		shader->setUniform("ks", glm::vec4(1.0f, 0, 0, 0));
		shader->setUniform("cam_tau", (glm::vec3(0.f)));
		shader->setUniform("cam_right", baseVectors[0]);
		shader->setUniform("cam_up", baseVectors[1]);
		shader->setUniform("cam_front", baseVectors[2]);
		shader->setUniform("discSize", glm::vec2(1) / discSize_);

		glActiveTexture(GL_TEXTURE0);
		deflectionTexture_->bind();
		glActiveTexture(GL_TEXTURE1);
		invRadiusTexture_->bind();
		glActiveTexture(GL_TEXTURE2);
		panoramaGrid_->bind();
	}
	else if (compareDeflection_) {
		glm::mat3 baseVectors = cam_.getBase3();
		starlessDeflectionShader_->use();
		starlessDeflectionShader_->setUniform("cam_right", baseVectors[0]);
		starlessDeflectionShader_->setUniform("cam_up", baseVectors[1]);
		starlessDeflectionShader_->setUniform("cam_front", baseVectors[2]);

		glActiveTexture(GL_TEXTURE2);
		panoramaGrid_->bind();
	} else {
			muellerDiscShader_->use();
			glm::vec3 cameraPos = cam_.getPositionRTP();
			muellerDiscShader_->setUniform("ri", cameraPos.x);
			muellerDiscShader_->setUniform("rsdri", 1.f/cameraPos.x);
			muellerDiscShader_->setUniform("iAngle", cameraPos.y);
			muellerDiscShader_->setUniform("discRadius", discSize_);
			muellerDiscShader_->setUniform("tfovXY", fovXY_);
	}


	glBindFramebuffer(GL_FRAMEBUFFER, fboTexture_->getFboId());
	glViewport(0, 0, fboTexture_->getWidth(), fboTexture_->getHeight());
	glClear(GL_COLOR_BUFFER_BIT);

	quad_.draw(GL_TRIANGLES);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_.getWidth(), window_.getHeight());
	glClear(GL_COLOR_BUFFER_BIT);

	sQuadShader_.use();

	glActiveTexture(GL_TEXTURE0);
	fboTexture_->bind();

	quad_.draw(GL_TRIANGLES);
}

void BHVApp::initShaders() {
	brunetonDiscShader_ = std::make_shared<Shader>(
		"test/disc_test_bruneton.vert", "test/disc_test_bruneton.frag");
	brunetonDeflectionShader_ = std::make_shared<Shader>(
		"test/disc_test_bruneton.vert", "test/defl_test_bruneton.frag");
	starlessDeflectionShader_ = std::make_shared<Shader>(
		"test/disc_test_bruneton.vert", "test/defl_test_starless.frag");
	brunetonDiscShader_->setBlockBinding("camera", CAMBINDING);
	brunetonDeflectionShader_->setBlockBinding("camera", CAMBINDING);
	starlessDeflectionShader_->setBlockBinding("camera", CAMBINDING);

	muellerDiscShader_ = std::make_shared<Shader>(
		"test/disc_test_mueller.vert", "test/disc_test_mueller.frag");
}

void BHVApp::initTextures() {

#pragma region deflection
	// create deflection texture
	std::vector<float> deflectionData = readFile<float>(TEX_DIR"ebruneton/deflection.dat");
	if (deflectionData.size() != 0) {

		TextureParams params;
		params.nrComponents = 2;
		params.width = deflectionData[0];
		params.height = deflectionData[1];
		params.internalFormat = GL_RG32F;
		params.format = GL_RG;
		params.type = GL_FLOAT;
		params.data = &deflectionData.data()[2];

		deflectionTexture_ = std::make_shared<Texture2D>(params);

		std::vector<std::pair<GLenum, GLint>> texParameters{
			{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
			{GL_TEXTURE_MAG_FILTER, GL_LINEAR}
		};
		deflectionTexture_->setParam(texParameters);

		std::cout << "Created deflection texture" << std::endl;
	}
	else {
		std::cerr << "[BHV App] Could not create deflection texture: no data read" << std::endl;
	}
#pragma endregion

#pragma region inverse radius
	// create inverse radius texture
	std::vector<float> invRadiusData = readFile<float>(TEX_DIR"ebruneton/inverse_radius_256x256.dat");
	if (invRadiusData.size() != 0) {

		TextureParams params;
		params.nrComponents = 2;
		params.width = invRadiusData[0];
		params.height = invRadiusData[1];
		params.internalFormat = GL_RG32F;
		params.format = GL_RG;
		params.type = GL_FLOAT;
		params.data = &invRadiusData.data()[2];

		invRadiusTexture_ = std::make_shared<Texture2D>(params);

		std::vector<std::pair<GLenum, GLint>> texParameters{
			{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
			{GL_TEXTURE_MAG_FILTER, GL_LINEAR}
		};
		invRadiusTexture_->setParam(texParameters);

		std::cout << "Created inverse radius texture" << std::endl;
	}
	else {
		std::cerr << "[BHV App] Could not create inverse radius texture: no data read" << std::endl;
	}
#pragma endregion

#pragma region panorama
	std::vector<std::string> grid{
		"gradient/right.png",
		"gradient/left.png",
		"gradient/top.png",
		"gradient/bottom.png",
		"gradient/front.png",
		"gradient/back.png"
	};
	panoramaGrid_ = std::make_shared<CubeMap>(grid);
#pragma endregion
}

void BHVApp::resizeTextures() {
	glm::vec2 dim{ window_.getWidth(), window_.getHeight() };
	fboTexture_->resize(fboScale_ * dim.x, fboScale_ * dim.y);
}

void BHVApp::calcFov(){
	fovXY_.x = glm::tan(0.5f * cam_.getFov()) * window_.getWidth() / window_.getHeight();
	fovXY_.y = glm::tan(0.5f * cam_.getFov());
}

void BHVApp::renderGui() {

	if (showFps_)
		renderFPSWindow();

	ImGui::Begin("Application Options");
	if (ImGui::BeginTabBar("Options")) {
		if (ImGui::BeginTabItem("General")) {

			// Saving, Loading State
			static std::string file;
			ImGui::Text("Save / Load current state");
			ImGui::PushItemWidth(ImGui::GetFontSize() * 7);
			ImGui::InputText("hi", &file);
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
		if (ImGui::BeginTabItem("Disc Settings")) {
			ImGui::SliderFloat2("Disc Dims", glm::value_ptr(discSize_), 3.f, 20.f);
			ImGui::EndTabItem();
		
		}
	}
	ImGui::EndTabBar();
	ImGui::End();
}

void BHVApp::renderShaderTab() {
	static int deflection = 0, shader = 0;
	ImGui::RadioButton("Disc", &deflection, 0);
	ImGui::RadioButton("Deflection", &deflection, 1);
	compareDeflection_ = (deflection == 1);
	
	ImGui::Separator();

	if (compareDeflection_) {
		ImGui::RadioButton("Bruneton", &shader, 0);
		ImGui::RadioButton("Starless", &shader, 1);
	}
	else {
		ImGui::RadioButton("Bruneton", &shader, 0);
		ImGui::RadioButton("Müller", &shader, 1);
	}
	showBruneton_ = (shader == 0);


	if (ImGui::Button("Reload Shaders")) {

		brunetonDiscShader_->reload();
		brunetonDeflectionShader_->reload();
		starlessDeflectionShader_->reload();
		muellerDiscShader_->reload();
		sQuadShader_.reload();
		brunetonDiscShader_->setBlockBinding("camera", CAMBINDING);
		brunetonDeflectionShader_->setBlockBinding("camera", CAMBINDING);
		starlessDeflectionShader_->setBlockBinding("camera", CAMBINDING);
	}

}

void BHVApp::renderCameraTab() {

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

void BHVApp::dumpState(std::string const& file) {
	
	return;
}

void BHVApp::readState(std::string const& file) {

	
	return;
}

void BHVApp::processKeyboardInput() {

	auto win = window_.getPtr();
	if (glfwGetKey(win, GLFW_KEY_G) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		showGui_ = true;
	} else if (glfwGetKey(win, GLFW_KEY_H) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		showGui_ = false;
	}
}

void BHVApp::printDebug() {
	//std::cout << "Nothing to do here :)" << std::endl;
	std::cout << glm::to_string(glm::vec2(1) / discSize_) << std::endl;


}