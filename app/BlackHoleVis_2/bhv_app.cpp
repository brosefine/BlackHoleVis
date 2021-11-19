#include <fstream>
#include <algorithm>
#include <numeric>
#include <functional>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <bhv_app.h>
#include <rendering/quad.h>
#include <helpers/RootDir.h>
#include <helpers/uboBindings.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

BHVApp::BHVApp(int width, int height)
	: GLApp(width, height, "Black Hole Vis")
	, cam_({ 0.f, 0.f, -10.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f })
	, camOrbit_(false)
	, camOrbitTilt_(0.f)
	, camOrbitRad_(10.f)
	, camOrbitSpeed_(0.5f)
	, camOrbitAngle_(0.f)
	, sky_({ "milkyway2048/right.png", "milkyway2048/left.png", "milkyway2048/top.png", "milkyway2048/bottom.png", "milkyway2048/front.png", "milkyway2048/back.png" })
	, deflectionTexture_("ebruneton/deflection.png")
	, invRadiusTexture_("ebruneton/inverse_radius.png")
	, fboTexture_(width, height)
	, fboScale_(1)
	, quad_(quadPositions, quadUVs, quadIndices)
	, sQuadShader_("squad.vs", "squad.fs")
	, t0_(0.f), dt_(0.f), tPassed_(0.f)
	, measureFrameTime_(false)
	, measureTime_(1.f), measureStart_(0.f)
	, measureID_("")
	, measureFrameWindow_(1)
	, vSync_(true)
	, showShaders_(false)
	, showCamera_(false)
	, showFps_(false)
{
	showGui_ = true;
	cam_.update(window_.getWidth(), window_.getHeight());
	initShaders();
	//initTextures();
}

void BHVApp::renderContent() 
{
	double now = glfwGetTime();
	dt_ = now - t0_;
	t0_ = now;
	tPassed_ += dt_;

	if (measureFrameTime_) {
		frameTimes_.push_back(dt_);
		if (now - measureStart_ >= measureTime_)
			finalizeFrameTimeMeasure();
	}

	if (camOrbit_) {
		calculateCameraOrbit();
	} else {
		cam_.keyBoardInput(window_.getPtr(), dt_);
		cam_.mouseInput(window_.getPtr());
	}

	if (window_.hasChanged()) {
		resizeTextures();
		cam_.update(window_.getWidth(), window_.getHeight());

	} else if (cam_.hasChanged()) {
		cam_.update(window_.getWidth(), window_.getHeight());
	}

	shader_->use();
	shader_->setUniform("cam_up", glm::normalize(cam_.getUp()));
	shader_->setUniform("cam_front", glm::normalize(cam_.getFront()));
	shader_->setUniform("cam_right", glm::normalize(cam_.getRight()));

	glActiveTexture(GL_TEXTURE0);
	deflectionTexture_.bind();
	glActiveTexture(GL_TEXTURE1);
	sky_.bind();

	glBindFramebuffer(GL_FRAMEBUFFER, fboTexture_.getFboId());
	glViewport(0, 0, fboTexture_.getWidth(), fboTexture_.getHeight());
	glClear(GL_COLOR_BUFFER_BIT);

	quad_.draw(GL_TRIANGLES);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_.getWidth(), window_.getHeight());
	glClear(GL_COLOR_BUFFER_BIT);

	sQuadShader_.use();

	glActiveTexture(GL_TEXTURE0);
	fboTexture_.bindTex();

	quad_.draw(GL_TRIANGLES);

}

void BHVApp::initShaders() {
	shaderElement_ = std::make_shared<BlackHoleShaderGui>();
	shader_ = shaderElement_->getShader();
}

void BHVApp::initTextures() {
	deflectionTexture_.setParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	deflectionTexture_.setParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	invRadiusTexture_.setParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	invRadiusTexture_.setParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void BHVApp::resizeTextures() {
	std::vector<int> dim{ window_.getWidth(), window_.getHeight() };
	fboTexture_.resize(fboScale_ * dim.at(0), fboScale_ * dim.at(1));
}

void BHVApp::calculateCameraOrbit() {
	glm::vec4 pos {
		std::cos(glm::radians(camOrbitAngle_)), //x
		0.f,									//y
		std::sin(glm::radians(camOrbitAngle_)), //z
		0.f
	};

	pos = glm::rotate(glm::radians(camOrbitTilt_), glm::vec3{ 0.f, 0.f, 1.f }) * pos;

	cam_.setPos(glm::vec3(pos) * camOrbitRad_);
	cam_.setFront(glm::vec3(pos) * -camOrbitRad_);

	camOrbitAngle_ += camOrbitSpeed_ * dt_;
	camOrbitAngle_ -= (camOrbitAngle_ > 360) * 360.f;

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

			// FPS plots and measurement
			ImGui::Text("Frame Time Measuring");
			ImGui::InputText("Measurement ID", &measureID_);
			ImGui::InputFloat("Duration", &measureTime_);
			ImGui::InputInt("Summarize x Frames", &measureFrameWindow_);
			if (ImGui::Button("Start Measuring"))
				initFrameTimeMeasure();

			ImGui::Checkbox("Show FPS", &showFps_);
			ImGui::Spacing();
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

void BHVApp::renderShaderTab() {
	shaderElement_->show();
}

void BHVApp::renderCameraTab() {

	ImGui::Text("Camera Settings");
	ImGui::Text("Camera Position");

	if (ImGui::Checkbox("Camera Orbit", &camOrbit_)) {
		camOrbitRad_ = glm::length(cam_.getPosition());
	}

	if (camOrbit_) {

		ImGui::SliderFloat("Orbit tilt", &camOrbitTilt_, 0.f, 89.f);
		ImGui::SliderFloat("Orbit speed", &camOrbitSpeed_, 0.f, 20.f);
		ImGui::SliderFloat("Orbit distance", &camOrbitRad_, 1.f, 100.f);
	} else {

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
}

void BHVApp::renderFPSWindow() {
	ImGui::Begin("FPS");
	static ScrollingBuffer rdata1, rdata2;
	static float t = 0;
	t += ImGui::GetIO().DeltaTime;
	rdata1.AddPoint(t, dt_);
	rdata2.AddPoint(t, (1.0f/dt_));

	static float history = 10.0f;
	ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");
	static ImPlotAxisFlags flags = ImPlotAxisFlags_AutoFit;
	
	ImGui::BulletText("Time btw. frames");

	ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
	if (ImPlot::BeginPlot("##Rolling1", NULL, NULL, ImVec2(-1, 150), 0, flags, flags)) {
		ImPlot::PlotLine("dt", &rdata1.Data[0].x, &rdata1.Data[0].y, rdata1.Data.size(), 0, 2 * sizeof(float));
		ImPlot::EndPlot();
	}

	ImGui::BulletText("Frames per Second");

	ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
	ImPlot::SetNextPlotLimitsY(0, 200, ImGuiCond_Always);
	if (ImPlot::BeginPlot("##Rolling2", NULL, NULL, ImVec2(-1, 150), 0, flags, flags)) {
		ImPlot::PlotLine("FPS", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 2 * sizeof(float));
		ImPlot::EndPlot();
	}

	ImGui::End();
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

void BHVApp::initFrameTimeMeasure() {
	// prepare vector containing frame times
	frameTimes_.clear();
	// assume 100 frames per second
	frameTimes_.reserve(100 * (measureTime_ + 1));

	measureFrameTime_ = true;
	showGui_ = false;
	measureStart_ = glfwGetTime();
}

void BHVApp::finalizeFrameTimeMeasure() {
	measureFrameTime_ = false;

	std::ofstream outIDFile(ROOT_DIR "data/measure_ids.txt", std::ios_base::app);
	outIDFile << "\n" << measureID_ << std::endl;
	outIDFile << "# " << frameTimes_.size() << " frames" << std::endl;
	outIDFile << "# " << measureTime_ << " seconds" << std::endl;
	outIDFile << "# avg over " << measureFrameWindow_ << " frames" << std::endl;
	outIDFile.close();

	std::ofstream outDataFile(ROOT_DIR "data/measure_data.txt", std::ios_base::app);

	int frameCount = 0;
	for (auto i = frameTimes_.begin(); i < frameTimes_.end(); i += measureFrameWindow_) {
		auto last = (frameCount + measureFrameWindow_) > frameTimes_.size() ?
			frameTimes_.end() : i + measureFrameWindow_;

		int nbFrames = last - i;
		
		// compute median
		float med;
		std::sort(i, last);
		if (nbFrames % 2) {
			med = 0.5f * (*(i + nbFrames / 2) + *(i + nbFrames / 2 - 1));
		} else {
			med = *(i + nbFrames / 2);
		}
		
		// compute average
		float avg = std::accumulate(i, last, 0.f) / nbFrames;

		// output is "id,frames,nbframes,min,max,med,avg,avgFPS\n";
		outDataFile << measureID_
			<< "," << frameCount
			<< "-" << std::min(frameCount + measureFrameWindow_, (int)frameTimes_.size()) - 1
			<< "," << nbFrames
			<< "," << *std::min_element(i, last)
			<< "," << *std::max_element(i, last)
			<< "," << med
			<< "," << avg
			<< "," << 1.f / avg
			<< "\n";

		frameCount += measureFrameWindow_;
	}

	outDataFile.close();
	showGui_ = true;
	measureID_ = "";
}

