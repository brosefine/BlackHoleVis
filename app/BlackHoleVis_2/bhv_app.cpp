#include <fstream>
#include <algorithm>
#include <numeric>
#include <functional>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <bhv_app.h>
#include <rendering/quad.h>
#include <helpers/RootDir.h>
#include <helpers/uboBindings.h>
#include <helpers/Timer.hpp>

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
	, camOrbit_(false)
	, camOrbitTilt_(0.f)
	, camOrbitRad_(10.f)
	, camOrbitSpeed_(0.5f)
	, camOrbitAngle_(0.f)
	, aberration_(false)
	, fido_(false)
	, useCustomDirection_(false)
	, useLocalDirection_(true)
	, direction_(1,0,0)
	, speed_(0.1f)
	, disc_(std::make_shared<ParticleDiscGui>())
	, diskTexture_(std::make_shared<Texture>("accretion1.jpg"))
	, deflectionPath_("ebruneton/deflection.dat")
	, invRadiusPath_("ebruneton/inverse_radius.dat")
	, blackBodyPath_("ebruneton/black_body.dat")
	, noiseTexture_(std::make_shared<Texture>("ebruneton/noise_texture.png"))
	, fboTexture_(std::make_shared<FBOTexture>(width, height))
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
	initTextures();
	initCubeMaps();
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
		cam_.processInput(window_.getPtr(), dt_);
	}

	if (window_.hasChanged()) {
		resizeTextures();
		cam_.update(window_.getWidth(), window_.getHeight());

	} else if (cam_.hasChanged()) {
		cam_.update(window_.getWidth(), window_.getHeight());
		uploadBaseVectors();
	}
	else if (aberration_) {
		uploadBaseVectors();
	}

	shader_->use();
	shader_->setUniform("dt", (float)tPassed_);

	glActiveTexture(GL_TEXTURE0);
	currentCubeMap_->bind();
	glActiveTexture(GL_TEXTURE1);
	deflectionTexture_->bind();
	glActiveTexture(GL_TEXTURE2);
	invRadiusTexture_->bind();
	glActiveTexture(GL_TEXTURE3);
	diskTexture_->bind();
	glActiveTexture(GL_TEXTURE4);
	blackBodyTexture_->bind();
	glActiveTexture(GL_TEXTURE5);
	noiseTexture_->bind();

	glBindFramebuffer(GL_FRAMEBUFFER, fboTexture_->getFboId());
	glViewport(0, 0, fboTexture_->getWidth(), fboTexture_->getHeight());
	glClear(GL_COLOR_BUFFER_BIT);

	quad_.draw(GL_TRIANGLES);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_.getWidth(), window_.getHeight());
	glClear(GL_COLOR_BUFFER_BIT);

	sQuadShader_.use();

	fboTexture_->generateMipMap();
	glActiveTexture(GL_TEXTURE0);
	fboTexture_->bind();

	quad_.draw(GL_TRIANGLES);

}

void BHVApp::initShaders() {
	shaderElement_ = std::make_shared<BlackHoleShaderGui>();
	shader_ = shaderElement_->getShader();
}

void BHVApp::initTextures() {

	// create deflection texture
	std::vector<float> deflectionData = readFile<float>(TEX_DIR"" + deflectionPath_);
	if (deflectionData.size() != 0) {

		TextureParams params;
		params.nrComponents = 2;
		params.width = deflectionData[0];
		params.height = deflectionData[1];
		params.internalFormat = GL_RG32F;
		params.format = GL_RG;
		params.type = GL_FLOAT;
		params.data = &deflectionData.data()[2];

		deflectionTexture_ = std::make_shared<Texture>(params);

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

	// create inverse radius texture
	std::vector<float> invRadiusData = readFile<float>(TEX_DIR"" + invRadiusPath_);
	if (invRadiusData.size() != 0) {

		TextureParams params;
		params.nrComponents = 2;
		params.width = invRadiusData[0];
		params.height = invRadiusData[1];
		params.internalFormat = GL_RG32F;
		params.format = GL_RG;
		params.type = GL_FLOAT;
		params.data = &invRadiusData.data()[2];

		invRadiusTexture_ = std::make_shared<Texture>(params);

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

	// create black body texture
	std::vector<float> blackBodyData = readFile<float>(TEX_DIR"" + blackBodyPath_);
	if (blackBodyData.size() != 0) {

		TextureParams params;
		params.nrComponents = 3;
		params.width = 128;
		params.height = 1;
		params.internalFormat = GL_RGB32F;
		params.format = GL_RGB;
		params.type = GL_FLOAT;
		params.data = blackBodyData.data();

		blackBodyTexture_ = std::make_shared<Texture>(params);

		std::vector<std::pair<GLenum, GLint>> texParameters{
			{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
			{GL_TEXTURE_MAG_FILTER, GL_LINEAR}
		};
		blackBodyTexture_->setParam(texParameters);

		std::cout << "Created black body texture" << std::endl;
	}
	else {
		std::cerr << "[BHV App] Could not create black body texture: no data read" << std::endl;
	}

	fboTexture_->generateMipMap();
	fboTexture_->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	std::vector<std::pair<GLenum, GLint>> texParameters{
		{GL_TEXTURE_WRAP_S, GL_REPEAT },
		{GL_TEXTURE_WRAP_T, GL_REPEAT },
		{GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR },
		{GL_TEXTURE_MAG_FILTER, GL_LINEAR }
	};
	noiseTexture_->setParam(texParameters);
	noiseTexture_->generateMipMap();
}

void BHVApp::initCubeMaps(){

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

void BHVApp::resizeTextures() {
	std::vector<int> dim{ window_.getWidth(), window_.getHeight() };
	fboTexture_->resize(fboScale_ * dim.at(0), fboScale_ * dim.at(1));
	fboTexture_->generateMipMap();
	fboTexture_->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
}

void BHVApp::calculateCameraOrbit() {
	glm::vec4 pos {
		std::cos(glm::radians(camOrbitAngle_)), //x
		0.f,									//y
		std::sin(glm::radians(camOrbitAngle_)), //z
		0.f
	};

	pos = glm::rotate(glm::radians(camOrbitTilt_), glm::vec3{ 0.f, 0.f, 1.f }) * pos;

	cam_.setPosXYZ(glm::vec3(pos) * camOrbitRad_);
	cam_.setViewDirXYZ(glm::vec3(pos) * -camOrbitRad_);

	camOrbitAngle_ += camOrbitSpeed_ * dt_;
	camOrbitAngle_ -= (camOrbitAngle_ > 360) * 360.f;

}

void BHVApp::uploadBaseVectors() {
	if (!aberration_) {
		
		glm::mat3 baseVectors = cam_.getBase3();
		shader_->use();
		shader_->setUniform("ks", glm::vec4(1.0f, 0, 0, 0));

		shader_->setUniform("cam_tau", (glm::vec3(0.f)));
		shader_->setUniform("cam_right", baseVectors[0]);
		shader_->setUniform("cam_up", baseVectors[1]);
		shader_->setUniform("cam_front", baseVectors[2]);
		return;
	}

	glm::mat4 lorentz;
	if (useCustomDirection_) {
		lorentz = cam_.getBoostFromVel(glm::normalize(direction_), speed_);
	} else if (useLocalDirection_) {
		lorentz = fido_ ? cam_.getBoostLocalFido(dt_) : cam_.getBoostLocal(dt_);
	} else {
		lorentz = cam_.getBoostGlobal(dt_);
	}
		
	glm::vec4 e_tau, e_right, e_up, e_front;
	glm::mat4 e_static = fido_ ? cam_.getFidoBase4() : cam_.getBase4();
	if (useLocalDirection_ || useCustomDirection_) {
		e_tau = e_static * lorentz[0];
		e_right = e_static * lorentz[1];
		e_up = e_static * lorentz[2];
		e_front = e_static * lorentz[3];
	} else {
		e_tau = lorentz * e_static[0];
		e_right = lorentz * e_static[1];
		e_up = lorentz * e_static[2];
		e_front = lorentz * e_static[3];
	}

	glm::vec3 camRTP = cam_.getPositionRTP();
	float u = 1.0f / camRTP.x;
	float v = glm::sqrt(1.0f - u);
	float sinT = glm::sin(camRTP.y);
	glm::vec4 ks(lorentz[0].x / v, lorentz[0].y * u / sinT, lorentz[0].z * u, lorentz[0].w * v);

	
	shader_->use();
	shader_->setUniform("cam_tau", (glm::vec3(e_tau.y, e_tau.z, e_tau.w)));
	shader_->setUniform("cam_right", (glm::vec3(e_right.y, e_right.z, e_right.w)));
	shader_->setUniform("cam_up", (glm::vec3(e_up.y, e_up.z, e_up.w)));
	shader_->setUniform("cam_front", (glm::vec3(e_front.y, e_front.z, e_front.w)));
	shader_->setUniform("ks", ks);
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
		if (ImGui::BeginTabItem("Sky Settings")) {
			renderSkyTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Disc Settings")) {
			disc_->show();
			ImGui::EndTabItem();
		}
	}
	ImGui::EndTabBar();
	ImGui::End();
}

void BHVApp::renderShaderTab() {
	shaderElement_->show();

	ImGui::Separator();
	ImGui::Text("Screen Quad Shader");
	if (ImGui::Button("Reload"))
		sQuadShader_.reload();
}

void BHVApp::renderCameraTab() {

	ImGui::Text("Camera Settings");

	ImGui::Separator();
	ImGui::Checkbox("Aberration", &aberration_);
	ImGui::Checkbox("Use custom velocity", &useCustomDirection_);
	if (useCustomDirection_) {
		ImGui::SliderFloat3("Direction", glm::value_ptr(direction_), -1.f, 1.0f);
		ImGui::SliderFloat("Speed", &speed_, 0.f, 0.999f);
	}
	else {
		ImGui::Checkbox("Use Local reference frame", &useLocalDirection_);
	}
	float speedScale = cam_.getSpeedScale();
	if (ImGui::SliderFloat("Speed Scale", &speedScale, 0.f, 1.f))
		cam_.setSpeedScale(speedScale);
	
	ImGui::Separator();
	ImGui::Text("Camera Mode");
	ImGui::Checkbox("Real Fido Frame", &fido_);
	static bool friction = true;
	ImGui::SameLine();
	if (ImGui::Checkbox("Friction", &friction))
		cam_.setFriction(friction);

	ImGui::Separator();
	ImGui::Text("Camera Position");

	if (ImGui::Checkbox("Camera Orbit", &camOrbit_)) {
		camOrbitRad_ = glm::length(cam_.getPositionXYZ());
	}

	if (camOrbit_) {

		ImGui::SliderFloat("Orbit tilt", &camOrbitTilt_, 0.f, 89.f);
		ImGui::SliderFloat("Orbit speed", &camOrbitSpeed_, 0.f, 20.f);
		ImGui::SliderFloat("Orbit distance", &camOrbitRad_, 1.f, 100.f);
	} else {

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
}

void BHVApp::renderSkyTab() {
	ImGui::Text("Sky Settings");

	bool skyChange = false;
	ImGui::Separator();
	ImGui::Text("Select CubeMap");
	static int cubemap = 0;
	int index = 0;
	for (const auto & [name, map] : cubemaps_) {
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
		switch (mipMap)	{
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
	if(maxSample < 0) glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxSample);
	if (skyChange || ImGui::SliderFloat("#Samples", &aniSample, 1.0f, maxSample)) {
		currentCubeMap_->setParam(GL_TEXTURE_MAX_ANISOTROPY, aniSample);
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

void BHVApp::printDebug() {
	//std::cout << "Nothing to do here :)" << std::endl;
	glm::mat3 base = cam_.getBase3();

	glm::mat4 lorentz = cam_.getBoostLocal(dt_);

	glm::vec4 e_tau, e_right, e_up, e_front;
	glm::mat4 e_static = cam_.getBase4();
	e_tau = e_static * lorentz[0];
	e_right = e_static * lorentz[1];
	e_up = e_static * lorentz[2];
	e_front = e_static * lorentz[3];

	glm::vec3 camRTP = cam_.getPositionRTP();
	float u = 1.0f / camRTP.x;
	float v = glm::sqrt(1.0f - u);
	float sinT = glm::sin(camRTP.y);
	glm::vec4 ks(lorentz[0].x, lorentz[0].y, lorentz[0].z, lorentz[0].w);
	glm::vec4 ks1(lorentz[0].x / v, lorentz[0].y * u / sinT, lorentz[0].z * u, lorentz[0].w * v);

	std::cout << "etau: " << glm::to_string(e_tau) << std::endl;
	std::cout << "ks: " << glm::to_string(ks) << std::endl;
	std::cout << "ks scaled: " << glm::to_string(ks1) << std::endl;

}