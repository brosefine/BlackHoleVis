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

// #define PRESENTATION_HELPER

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
	, fboTexture_(width, height)
	, sQuadShader_("squad.vs", "squad.fs")
	, fboScale_(1)
	, bloom_(false)
	, diskRotationSpeed_(10.f)
	, disk_(DISKBINDING)
	, selectedTexture_("")
	, selectedComputeShader_(0)
	, selectedShader_(0)
	, compute_(false)
	, t0_(0.f), dt_(0.f), tPassed_(0.f)
	, measureFrameTime_(false)
	, measureTime_(1.f), measureStart_(0.f)
	, measureID_("")
	, measureFrameWindow_(1)
	, vSync_(true)
	, showGui_ (true)
	, showShaders_(false)
	, showCamera_(false)
	, showDisk_(false)
	, showFps_(false)
{
	cam_.update(window_.getWidth(), window_.getHeight());
	initGuiElements();
	initTextures();
}

static int nextPowerOfTwo(int x) {
	x--;
	x |= x >> 1; // handle 2 bit numbers
	x |= x >> 2; // handle 4 bit numbers
	x |= x >> 4; // handle 8 bit numbers
	x |= x >> 8; // handle 16 bit numbers
	x |= x >> 16; // handle 32 bit numbers
	x++;
	return x;
}

void BHVApp::renderLoop() {
	glClearColor(0.5, 0.5, 0.5, 1.0);

	while (!window_.shouldClose()) {

		double now = glfwGetTime();
		dt_ = now - t0_;
		t0_ = now;
		tPassed_ += dt_;
		tPassed_ -= (tPassed_ > diskRotationSpeed_) * diskRotationSpeed_;

		if (measureFrameTime_) {
			frameTimes_.push_back(dt_);
			if (now - measureStart_ >= measureTime_)
				finalizeFrameTimeMeasure();
		}
		
		processKeyboardInput();
		if (showGui_) {

			gui_.newFrame();
			renderOptionsWindow();
			gui_.render();
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
			updateComputeUniforms();

		} else if (cam_.hasChanged()) {
			cam_.update(window_.getWidth(), window_.getHeight());
			updateComputeUniforms();
		}

		getCurrentShader()->use();

		glActiveTexture(GL_TEXTURE0);
		sky_.bind();

		glActiveTexture(GL_TEXTURE1);
		diskTextures_.at(selectedTexture_)->bind();

		disk_.setRotation(tPassed_ / diskRotationSpeed_);
		disk_.uploadData();
		
		if (compute_) {
			bloom_ = getCurrentShader()->getFlags().at("BLOOM");
			glActiveTexture(GL_TEXTURE0);
			fboTexture_.bindImageTex(0, GL_WRITE_ONLY);
			bloomTextures_.at(0)->bindImageTex(1, GL_WRITE_ONLY);
			glm::ivec3 workGroups;
			glGetProgramiv(getCurrentShader()->getID(), GL_COMPUTE_WORK_GROUP_SIZE, glm::value_ptr(workGroups));
			glDispatchCompute(
				nextPowerOfTwo(std::ceil(fboTexture_.getWidth() / (float)workGroups.x)),
				nextPowerOfTwo(std::ceil(fboTexture_.getHeight() / (float)workGroups.y)), 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		} else {

			glBindFramebuffer(GL_FRAMEBUFFER, fboTexture_.getFboId());
			glViewport(0, 0, fboTexture_.getWidth(), fboTexture_.getHeight());
			glClear(GL_COLOR_BUFFER_BIT);

			quad_.draw(GL_TRIANGLES);

		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, window_.getWidth(), window_.getHeight());
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		if (bloom_) 
			bloomTextures_.at(0)->bindTex();
		else 
			fboTexture_.bindTex();

		sQuadShader_.use();
		quad_.draw(GL_TRIANGLES);
				
		if(showGui_) gui_.renderEnd();

		window_.endFrame();
	}
}

std::shared_ptr<ShaderBase> BHVApp::getCurrentShader() {
	if(compute_)
		return computeShaderElements_.at(selectedComputeShader_)->getShader();
	return shaderElements_.at(selectedShader_)->getShader();
}

void BHVApp::initGuiElements() {
	shaderElements_.push_back(std::make_shared<NewtonShaderGui>());
	shaderElements_.push_back(std::make_shared<StarlessShaderGui>());
	shaderElements_.push_back(std::make_shared<TestShaderGui>());

	computeShaderElements_.push_back(std::make_shared<StarlessComputeShaderGui>());
}

void BHVApp::initTextures() {
	selectedTexture_ = "Fine Texture";
	diskTextures_.insert({ "Fine Texture", std::make_shared<Texture>("accretion.jpg") });
	diskTextures_.insert({ "Blurred Texture", std::make_shared<Texture>("accretion1.jpg") });

	bloomTextures_.push_back(std::make_shared<FBOTexture>(window_.getWidth(), window_.getHeight()));
	bloomTextures_.push_back(std::make_shared<FBOTexture>(window_.getWidth(), window_.getHeight()));
}

void BHVApp::resizeTextures() {
	std::vector<int> dim{ window_.getWidth(), window_.getHeight() };
	fboTexture_.resize(fboScale_ * dim.at(0), fboScale_ * dim.at(1));
	for (auto& tex : bloomTextures_) {
		tex->resize(fboScale_ * dim.at(0), fboScale_ * dim.at(1));
	}
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
		if (ImGui::BeginTabItem("Compute Shader Settings")) {
			renderComputeWindow();
			ImGui::EndTabItem();
		}
	}
	ImGui::EndTabBar();
	ImGui::End();
}

void BHVApp::renderShaderWindow() {
	ImGui::Text("Shader Settings");
	ImGui::Checkbox("Use compute shader", &compute_);
	ImGui::Text("Shader Selection");
	if (compute_) {
		if (ImGui::BeginListBox("")) {

			for (int i = 0; i < computeShaderElements_.size(); ++i) {
				auto shader = computeShaderElements_.at(i);
				if (ImGui::Selectable(shader->getName().c_str(), false)) {

					selectedComputeShader_ = i;
					computeShaderElements_.at(i)->updateShader();
				}
			}
			ImGui::EndListBox();
		}
		ImGui::Separator();
		computeShaderElements_.at(selectedComputeShader_)->show();

	} else {

		if (ImGui::BeginListBox("")) {

			for (int i = 0; i < shaderElements_.size(); ++i) {
				auto shader = shaderElements_.at(i);
				if (ImGui::Selectable(shader->getName().c_str(), false)) {

					selectedShader_ = i;
					shaderElements_.at(i)->updateShader();
				}
			}
			ImGui::EndListBox();
		}
		ImGui::Separator();
		shaderElements_.at(selectedShader_)->show();

	}
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
	ImPlot::SetNextPlotLimitsY(0, 200, ImGuiCond_Always);
	if (ImPlot::BeginPlot("##Rolling2", NULL, NULL, ImVec2(-1, 150), 0, flags, flags)) {
		ImPlot::PlotLine("FPS", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 2 * sizeof(float));
		ImPlot::EndPlot();
	}
}

void BHVApp::renderComputeWindow() {
	if (ImGui::Button("Print Info")) printComputeInfo();
}

void BHVApp::dumpState(std::string const& file) {

	std::ofstream outFile(ROOT_DIR "saves/" + file);
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

	outFile << "ComputeShaders\n";
	for (int i = 0; i < computeShaderElements_.size(); ++i) {
		outFile << "ComputeShader " << i << "\n";
		computeShaderElements_.at(i)->dumpState(outFile);
		outFile << "EndShader\n";
	}

	outFile << "Screen\n";
	outFile << window_.getWidth() << " " << window_.getHeight() << " " << fboScale_ << "\n";

	outFile.close();
}

void BHVApp::readState(std::string const& file) {

	std::ifstream inFile(ROOT_DIR "saves/" + file);
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

		if (word == "ComputeShaders") {
			inFile >> word;
			while (word == "ComputeShader") {
				inFile >> word;
				computeShaderElements_.at(std::stoi(word))->readState(inFile);
				inFile >> word;
			}
		}

		if (word == "Screen") {
			inFile >> word; window_.setWidth(std::stoi(word));
			inFile >> word; window_.setHeight(std::stoi(word));
			inFile >> word; fboScale_ = std::stoi(word);
		}
	}

	inFile.close();
}

void BHVApp::processKeyboardInput() {

	auto win = window_.getPtr();
	if (glfwGetKey(win, GLFW_KEY_G) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		showGui_ = true;
	} else if (glfwGetKey(win, GLFW_KEY_H) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		showGui_ = false;
	}

	if (glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		bloom_ = !bloom_;
	}

#ifdef PRESENTATION_HELPER
	static bool step = false;
	static float weight = 0.f;
	if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) {
		step = true;
	}
	if (step && glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) {
		step = false;
		static int animationCount = 0;
		switch (animationCount++) {
		case 0:
			camOrbit_ = true;
			break;
		case 1:
			getCurrentShader()->setFlag("CHECKEREDHOR", true);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		case 2:
			getCurrentShader()->setFlag("SKY", true);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		case 3:
			getCurrentShader()->setFlag("DISK", true);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		case 4:
			getCurrentShader()->setFlag("CHECKEREDDISK", false);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		case 5:
			getCurrentShader()->setFlag("CHECKEREDHOR", false);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		case 6:
			camOrbit_ = false;
			break;
		case 7:
			camOrbit_ = true;
			break;
		case 8:
			getCurrentShader()->setFlag("CHECKEREDDISK", true);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		case 9:
			getCurrentShader()->setFlag("CHECKEREDHOR", true);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		case 10:
			getCurrentShader()->setFlag("SKY", false);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		case 11:
			getCurrentShader()->setFlag("DISK", false);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		case 12:
			getCurrentShader()->setFlag("CHECKEREDHOR", false);
			shaderElements_.at(selectedShader_)->updateShader();
			getCurrentShader()->setUniform("forceWeight", weight);

			break;
		default:
			animationCount = 0;
			break;
		}
		std::cout << "step: " << animationCount << std::endl;
	}

	if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) {
		weight = std::max(0.f, weight - 0.005f);
		getCurrentShader()->setUniform("forceWeight", weight);
	} else if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		weight = std::min(1.5f, weight + 0.005f);
		getCurrentShader()->setUniform("forceWeight", weight);
	}
#endif //PRESENTATION_HELPER
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

void BHVApp::printComputeInfo() {
// determining work group size and number as in
// https://antongerdelan.net/opengl/compute.html

	int info[3];

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &info[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &info[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &info[2]);

	printf("max global (total) work group counts x:%i y:%i z:%i\n",
		info[0], info[1], info[2]);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &info[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &info[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &info[2]);

	printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
		info[0], info[1], info[2]);

	int work_grp_inv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	printf("max local work group invocations %i\n", work_grp_inv);
}

void BHVApp::updateComputeUniforms() {

	// transform quad positions into world space
	std::vector<glm::vec4> positions;
	auto projViewInvMat = cam_.getData().projectionViewInverse_;
	for (auto const& v : quad_.vertices_) {
		positions.push_back((projViewInvMat * glm::vec4(v.position, 1.0f)));
	}

	// assemble matrix for shader
	glm::mat4 dataMat{ 0.0f };
	for (int i = 0; i < 4; ++i) {
		dataMat[i] = positions.at(i) / positions.at(i).w;
	}
	glm::vec3 camPos = cam_.getPosition();
	dataMat[0][3] = camPos.x;
	dataMat[1][3] = camPos.y;
	dataMat[2][3] = camPos.z;

	for (auto const& shader : computeShaderElements_) {
		shader->getShader()->setUniform("quadCamInfo", dataMat);
	}
}
