#include <fstream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp >

#include <bhv_app.h>
#include <rendering/quad.h>
#include <helpers/uboBindings.h>





BHVApp::BHVApp(int width, int height)
	: window_(width, height, "Black Hole Vis")
	, gui_(window_.getPtr())
	, cam_({ 0.f, 0.f, -10.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f })
	, camOrbit_(false)
	, camOrbitTilt_(0.f)
	, camOrbitRad_(10.f)
	, camOrbitSpeed_(0.5f)
	, camOrbitAngle_(0.f)
	, quad_(quadPositions, quadUVs, quadIndices)
	, sky_({ "sky_right.png", "sky_left.png", "sky_top.png", "sky_bottom.png", "sky_front.png", "sky_back.png" })
	, diskRotationSpeed_(10.f)
	, disk_(DISKBINDING)
	, selectedTexture_("")
	, selectedShader_(0)
	, t0_(0.f), dt_(0.f), tPassed_(0.f)
	, showShaders_(false)
	, showCamera_(false)
	, showDisk_(false)
	, showFps_(false)
{
	cam_.update(window_.getWidth(), window_.getHeight());
	initGuiElements();
	initDiskTextures();
}

void BHVApp::renderLoop() {

	while (!window_.shouldClose()) {

		gui_.newFrame();
		renderOptionsWindow();
		gui_.render();

		float now = glfwGetTime();
		dt_ = now - t0_;
		t0_ = now;
		tPassed_ += dt_;
		tPassed_ -= (tPassed_ > diskRotationSpeed_) * diskRotationSpeed_;

		if (camOrbit_) {
			calculateCameraOrbit();
		} else {
			cam_.keyBoardInput(window_.getPtr(), dt_);
			cam_.mouseInput(window_.getPtr());

		}

		if (cam_.hasChanged() || window_.hasChanged())
			cam_.update(window_.getWidth(), window_.getHeight());

		glClearColor(0.5, 0.5, 0.5, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		sky_.bind();

		glActiveTexture(GL_TEXTURE1);
		diskTextures_.at(selectedTexture_)->bind();

		getCurrentShader()->use();
		
		disk_.setRotation(tPassed_ / diskRotationSpeed_);
		disk_.uploadData();
		quad_.draw(GL_TRIANGLES);
		
		gui_.renderEnd();
		window_.endFrame();
	}
}

std::shared_ptr<Shader> BHVApp::getCurrentShader() {
	return shaderElements_.at(selectedShader_)->getShader();
}

void BHVApp::initGuiElements() {
	shaderElements_.push_back(std::make_shared<NewtonShaderGui>());
	shaderElements_.push_back(std::make_shared<StarlessShaderGui>());
	shaderElements_.push_back(std::make_shared<TestShaderGui>());
}

void BHVApp::initDiskTextures() {
	selectedTexture_ = "Fine Texture";
	diskTextures_.insert({ "Fine Texture", std::make_shared<Texture>("accretion.jpg") });
	diskTextures_.insert({ "Blurred Texture", std::make_shared<Texture>("accretion1.jpg") });
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

void BHVApp::renderOptionsWindow() {
	ImGui::Begin("Application Options");
	if (ImGui::BeginTabBar("Options")) {
		if (ImGui::BeginTabItem("General")) {
			ImGui::Text("Save / Load current state");
			ImGui::PushItemWidth(ImGui::GetFontSize() * 7);
			if (ImGui::Button("Save")) dumpState();
			ImGui::SameLine();
			if (ImGui::Button("Load")) readState();
			ImGui::PopItemWidth();
			ImGui::Checkbox("Show FPS", &showFps_);
			ImGui::Spacing();
			if(showFps_)
				renderFPSPlot();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Shader Settings")) {
			renderShaderWindow();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Camera Settings")) {
			renderCameraWindow();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Accretion Disk Settings")) {
			renderDiskWindow();
			ImGui::EndTabItem();
		}
	}

	ImGui::End();
}

void BHVApp::renderShaderWindow() {
	ImGui::Text("Shader Settings");
	ImGui::Text("Shader Selection");
	if (ImGui::BeginListBox("")) {

		for (int i = 0; i < shaderElements_.size(); ++i) {
			auto shader = shaderElements_.at(i);
			if (ImGui::Selectable(shader->getName().c_str(), false))
				selectedShader_ = i;
		}
		ImGui::EndListBox();
	}
	ImGui::Separator();
	shaderElements_.at(selectedShader_)->show();

}

void BHVApp::renderCameraWindow() {

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

void BHVApp::renderDiskWindow() {
	ImGui::Text("Accretion Disk Settings");
	
	glm::vec2 accretionDim{ disk_.getMinRad(), disk_.getMaxRad() };
	if (ImGui::SliderFloat2("Disk Size", glm::value_ptr(accretionDim), 1.f, 20.f)) {
		disk_.setRad(accretionDim.x, accretionDim.y);
	}

	ImGui::SliderFloat("Disk Rotation Period", &diskRotationSpeed_, 1.f, 20.f);

	ImGui::Text("Texture Selection");
	if (ImGui::BeginListBox("")) {

		for (auto const& tex : diskTextures_) {
			if (ImGui::Selectable(tex.first.c_str(), false))
				selectedTexture_ = tex.first;
		}
		ImGui::EndListBox();
	}
}

void BHVApp::renderFPSPlot() {
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
	if (ImPlot::BeginPlot("##Rolling2", NULL, NULL, ImVec2(-1, 150), 0, flags, flags)) {
		ImPlot::PlotLine("FPS", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 2 * sizeof(float));
		ImPlot::EndPlot();
	}
}

void BHVApp::dumpState() {
	std::cout << "Dump" << std::endl;

	std::ofstream outFile("dump.txt");
	outFile << "Camera\n";
	glm::vec3 camPos = cam_.getPosition();
	outFile << camPos.x << " " << camPos.y << " " << camPos.z << "\n";
	glm::vec3 camFront = cam_.getFront();
	outFile << camFront.x << " " << camFront.y << " " << camFront.z << "\n";
	glm::vec3 camUp = cam_.getUp();
	outFile << camUp.x << " " << camUp.y << " " << camUp.z << "\n";
	outFile << camOrbitTilt_ << " " << camOrbitSpeed_ << "\n";

	outFile << "Disk\n";
	outFile << disk_.getMinRad() << " " << disk_.getMaxRad() << " " << diskRotationSpeed_ << "\n";

	outFile << "Shaders\n";
	for (int i = 0; i < shaderElements_.size(); ++i) {
		outFile << "Shader " << i << "\n";
		shaderElements_.at(i)->dumpState(outFile);
		outFile << "EndShader\n";
	}

	outFile.close();
}

void BHVApp::readState() {
	std::cout << "Read" << std::endl;

	std::ifstream inFile("dump.txt");
	std::string word;
	while (inFile >> word) {
		if (word == "Camera") {
			glm::vec3 camPos;
			inFile >> word; camPos.x = std::stof(word);
			inFile >> word; camPos.y = std::stof(word);
			inFile >> word; camPos.z = std::stof(word);
			glm::vec3 camFront;
			inFile >> word; camFront.x = std::stof(word);
			inFile >> word; camFront.y = std::stof(word);
			inFile >> word; camFront.z = std::stof(word);
			glm::vec3 camUp;
			inFile >> word; camUp.x = std::stof(word);
			inFile >> word; camUp.y = std::stof(word);
			inFile >> word; camUp.z = std::stof(word);
			inFile >> word; camOrbitTilt_ = std::stof(word);
			inFile >> word; camOrbitSpeed_ = std::stof(word);
			
			cam_.setPos(camPos);
			cam_.setFront(camFront);
			cam_.setUp(camUp);
		}
		if (word == "Disk") {
			glm::vec2 diskSize;
			inFile >> word; diskSize.x = std::stof(word);
			inFile >> word; diskSize.y = std::stof(word);
			inFile >> word; diskRotationSpeed_ = std::stof(word);
			disk_.setRad(diskSize.x, diskSize.y);
		}

		if (word == "Shaders") {
			inFile >> word;
			while (word == "Shader") {
				inFile >> word;
				shaderElements_.at(std::stoi(word))->readState(inFile);
				inFile >> word;
			}
		}
	}

	inFile.close();
}
