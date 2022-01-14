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

#include <envTest_app.h>
#include <helpers/RootDir.h>
#include <helpers/uboBindings.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>


EnvApp::EnvApp(int width, int height)
	: GLApp(width, height, "Black Hole Vis")
	, cam_({ 0.f, 0.f, 20.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, -1.f })
	, fboTexture_(std::make_shared<FBOTexture>(width, height))
	, mesh_(std::make_shared<Mesh>("sphere.obj"))
	, sQuadShader_(std::make_shared<Shader>("squad.vs", "squad.fs"))
	, skyShader_(std::make_shared<Shader>("envTest/sky.vs", "envTest/sky.fs"))
	, meshShader_(std::make_shared<Shader>("envTest/mesh.vs", "envTest/mesh.fs"))
	, t0_(0.f), dt_(0.f), tPassed_(0.f)
	, speedScale_(0.f)
	, vSync_(true)
	, showShaders_(false)
	, showCamera_(false)
{
	showGui_ = true;
	cam_.update(window_.getWidth(), window_.getHeight());
	reloadShaders();
	initTextures();
	//initShaders();
}

void EnvApp::renderContent() 
{
	double now = glfwGetTime();
	dt_ = now - t0_;
	t0_ = now;
	tPassed_ += dt_;

	cam_.keyBoardInput(window_.getPtr(), dt_);
	cam_.mouseInput(window_.getPtr());

	if (window_.hasChanged()) {
		resizeTextures();
		cam_.update(window_.getWidth(), window_.getHeight());

	} else if (cam_.hasChanged()) {
		cam_.update(window_.getWidth(), window_.getHeight());
	}

	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_.getWidth(), window_.getHeight());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	meshTexture_->bind();

	meshShader_->use();
	
	static float rotAngle = 0;
	rotAngle += speedScale_ * dt_;
	//glm::mat4 model = glm::rotate(glm::radians(rotAngle), glm::vec3(0, 1, 0)) * glm::translate(glm::vec3(10, 0, 0)) * glm::mat4();
	glm::mat4 earthModel = glm::rotate(glm::radians(rotAngle), glm::vec3(0, 1, 0)) * glm::translate(glm::vec3(10, 0, 0)) * glm::mat4(1);
	meshShader_->setUniform("modelMatrix", earthModel);
	mesh_->draw(GL_TRIANGLES);

	glm::mat4 moonModel = earthModel * glm::scale(glm::vec3(0.2f)) * earthModel;
	meshShader_->setUniform("modelMatrix", moonModel);
	mesh_->draw(GL_TRIANGLES);

	glActiveTexture(GL_TEXTURE0);
	skyTexture_->bind();

	skyShader_->use();
	quad_.draw(GL_TRIANGLES);
}

void EnvApp::initShaders() {
}

void EnvApp::reloadShaders() {
	sQuadShader_->reload();
	meshShader_->reload();
	skyShader_->reload();
	skyShader_->setBlockBinding("camera", CAMBINDING);
	meshShader_->setBlockBinding("camera", CAMBINDING);
}

void EnvApp::initTextures() {
	meshTexture_ = std::make_shared<Texture2D>("Earthmap.jpg");

	std::vector<std::string> skyFaces = { "milkyway2048/right.png", "milkyway2048/left.png", "milkyway2048/top.png", "milkyway2048/bottom.png", "milkyway2048/front.png", "milkyway2048/back.png" };
	skyTexture_ = std::make_shared<CubeMap>(skyFaces);
}

void EnvApp::resizeTextures() {
	//glm::vec2 dim{ window_.getWidth(), window_.getHeight() };
	//fboTexture_->resize(dim.x, dim.y);
}

void EnvApp::calcFov(){
}

void EnvApp::renderGui() {

	if (showFps_)
		renderFPSWindow();

	ImGui::Begin("Application Options");
	if (ImGui::BeginTabBar("Options")) {
		if (ImGui::BeginTabItem("General")) {


			ImGui::Text("Set Window Size");
			std::vector<int> dim {window_.getWidth(), window_.getHeight()};
			if (ImGui::InputInt2("window size", dim.data())) {
				window_.setWidth(dim.at(0));
				window_.setHeight(dim.at(1));
			}

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

void EnvApp::renderShaderTab() {

	ImGui::SliderFloat("Speed", &speedScale_, 0.f, 10.f);
	if (ImGui::Button("Reload Shaders")) {
		reloadShaders();
	}

}

void EnvApp::renderCameraTab() {

	ImGui::Text("Camera Settings");

	ImGui::Separator();
	ImGui::Text("Camera Position");

	glm::vec3 camPos = cam_.getPosition();
	ImGui::PushItemWidth(ImGui::GetFontSize() * 7);
	bool xChange = ImGui::InputFloat("X", &camPos.x, 0.1, 1); ImGui::SameLine();
	bool yChange = ImGui::InputFloat("Y", &camPos.y, 0.1, 1); ImGui::SameLine();
	bool zChange = ImGui::InputFloat("Z", &camPos.z, 0.1, 1);
	ImGui::PopItemWidth();

	if (ImGui::Button("Point to Black Hole"))
		cam_.setFront(-camPos);
	
	if (xChange || yChange || zChange)
		cam_.setPos(camPos);
}

void EnvApp::dumpState(std::string const& file) {
	
	return;
}

void EnvApp::readState(std::string const& file) {

	
	return;
}

void EnvApp::processKeyboardInput() {

	auto win = window_.getPtr();
	if (glfwGetKey(win, GLFW_KEY_G) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		showGui_ = true;
	} else if (glfwGetKey(win, GLFW_KEY_H) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		showGui_ = false;
	}
}

void EnvApp::printDebug() {
	std::cout << "Nothing to do here :)" << std::endl;

}