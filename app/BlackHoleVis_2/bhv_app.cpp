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
	, noiseTexture_(std::make_shared<Texture2D>("ebruneton/noise_texture.png"))
	, fboTexture_(std::make_shared<FBOTexture>(width, height))
	, fboScale_(1)
	, bloom_(false)
	, bloomEffect_(width, height, 9)
	, sQuadShader_("squad.vs", "squad.fs")
	, renderEnvironment_(false)
	, t0_(0.f), dt_(0.f), tPassed_(0.f)
	, vSync_(true)
	, showShaders_(false)
	, showCamera_(false)
{
	showGui_ = true;
	cam_.update(window_.getWidth(), window_.getHeight());
	cam_.use(window_.getWidth(), window_.getHeight());
	initShaders();
	initTextures();
	initCubeMaps();
	initScenes();
}

void BHVApp::renderContent() 
{
	double now = glfwGetTime();
	dt_ = now - t0_;
	t0_ = now;
	tPassed_ += dt_;

	if (camOrbit_) {
		calculateCameraOrbit();
	} else {
		cam_.processInput(window_.getPtr(), dt_);
	}

	static int turns = 0; 
	if (turns == 0 && renderEnvironment_) {
		currentEnvironmentScene_->render(cam_.getPositionXYZ(), dt_);
		cam_.use(window_.getWidth(), window_.getHeight(), false);
	}
	//turns = ++turns % 3; // update env only every 3rd frame

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

	if(bloom_)
		bloomEffect_.begin();

	shader_->use();
	shader_->setUniform("dt", (float)tPassed_);

	glActiveTexture(GL_TEXTURE0);
	if (renderEnvironment_) {
		shader_->setUniform("gaiaMap", false);
		currentEnvironmentScene_->bindEnv();
	}
	else {
		shader_->setUniform("gaiaMap", currentCubeMap_ == galaxyTexture_);
		currentCubeMap_->bind();
	}

	glActiveTexture(GL_TEXTURE1);
	deflectionTexture_->bind();
	glActiveTexture(GL_TEXTURE2);
	invRadiusTexture_->bind();
	glActiveTexture(GL_TEXTURE3);
	jetTexture_->bind();
	glActiveTexture(GL_TEXTURE4);
	blackBodyTexture_->bind();
	glActiveTexture(GL_TEXTURE5);
	dopplerTexture_->bind();
	glActiveTexture(GL_TEXTURE6);
	starTexture_->bind();
	glActiveTexture(GL_TEXTURE7);
	starTexture2_->bind();
	glActiveTexture(GL_TEXTURE8);
	noiseTexture_->bind();

	if (!bloom_) {

		glBindFramebuffer(GL_FRAMEBUFFER, fboTexture_->getFboId());
		glViewport(0, 0, fboTexture_->getWidth(), fboTexture_->getHeight());
	}
	glClear(GL_COLOR_BUFFER_BIT);

	quad_.draw(GL_TRIANGLES);

	if(bloom_)
		bloomEffect_.render(fboTexture_->getFboId());
	
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

	jetTexture_ = std::make_shared<Texture2D>("jet_noise.jpg");
	std::vector<std::pair<GLenum, GLint>> jettexParameters{
		{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE },
		{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE },
		{GL_TEXTURE_MIN_FILTER, GL_LINEAR },
		{GL_TEXTURE_MAG_FILTER, GL_LINEAR }
	};
	jetTexture_->setParam(jettexParameters);


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
	std::vector<float> invRadiusData = readFile<float>(TEX_DIR"ebruneton/inverse_radius.dat");
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

#pragma region black body
	// create black body texture
	std::vector<float> blackBodyData = readFile<float>(TEX_DIR"ebruneton/black_body.dat");
	if (blackBodyData.size() != 0) {

		TextureParams params;
		params.nrComponents = 3;
		params.width = 128;
		params.height = 1;
		params.internalFormat = GL_RGB32F;
		params.format = GL_RGB;
		params.type = GL_FLOAT;
		params.data = blackBodyData.data();

		blackBodyTexture_ = std::make_shared<Texture2D>(params);

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
#pragma endregion

#pragma region doppler
	// create doppler texture
	std::vector<float> dopplerData = readFile<float>(TEX_DIR"ebruneton/doppler.dat");
	//std::vector<float> dopplerData(3*64 * 32 * 64, 1.f);
	if (dopplerData.size() != 0) {

		TextureParams params;
		params.nrComponents = 3;
		params.width = 64;
		params.height = 32;
		params.depth = 64;
		params.internalFormat = GL_RGB32F;
		params.format = GL_RGB;
		params.type = GL_FLOAT;
		params.data = dopplerData.data();

		dopplerTexture_ = std::make_shared<Texture3D>(params);

		std::vector<std::pair<GLenum, GLint>> texParameters{
			{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
			{GL_TEXTURE_MAG_FILTER, GL_LINEAR}
		};
		dopplerTexture_->setParam(texParameters);

		std::cout << "Created doppler texture" << std::endl;
	}
	else {
		std::cerr << "[BHV App] Could not create black body texture: no data read" << std::endl;
	}
#pragma endregion

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
}

void BHVApp::initScenes()
{
	environmentScenes_.insert({ "Solar System", std::make_shared<SolarSystemScene>(2048) });
	environmentScenes_.insert({ "Checker Sphere", std::make_shared<CheckerSphereScene>(2048) });
	currentEnvironmentScene_ = environmentScenes_["Solar System"];
}

void BHVApp::loadStarTextures() {
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
	std::vector<std::string> faces {
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

void BHVApp::loadStarTile(int level, int ti, int tj, int face, int faceSize, int tileSize, GLenum target, std::string path){
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
			glTextureSubImage3D(starTexture2_->getTexId(), currentLevel - (MAX_STAR_LOD+1), ti * tileSize, tj * tileSize, face, tileSize, tileSize, 1,
				GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV, &tileData.data()[start]);
		}
		start += tileSize * tileSize;
		currentLevel++;
		tileSize /= 2;
	}
}

void BHVApp::resizeTextures() {
	glm::vec2 dim{ window_.getWidth(), window_.getHeight() };
	fboTexture_->resize(fboScale_ * dim.x, fboScale_ * dim.y);
	fboTexture_->generateMipMap();
	fboTexture_->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	bloomEffect_.resize(fboScale_ * dim.x, fboScale_ * dim.y);
}

void BHVApp::calculateCameraOrbit() {
	glm::vec4 pos {
		std::cos(glm::radians(camOrbitAngle_)), //x
		0.f,									//y
		std::sin(glm::radians(camOrbitAngle_)), //z
		0.f
	};

	pos = glm::rotate(glm::radians(camOrbitTilt_), glm::vec3{ 0.f, 0.f, 1.f }) * pos;

	cam_.setPosXYZ(glm::vec3(pos) * camOrbitRad_, false);
	cam_.setViewDirXYZ(-glm::vec3(pos));

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
	glm::vec4 ks = lorentz[0];

	if (!useLocalDirection_) {
		glm::mat3 globToLoc = glm::inverse(cam_.getBase3());
		ks = glm::vec4(
			ks.x,
			globToLoc * glm::vec3(ks.y, ks.z, ks.w)
		);
	}

	ks.x /= v;
	ks.y *= u / sinT;
	ks.z *= u;
	ks.w *= v;

	
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
		if (ImGui::BeginTabItem("Bloom Settings")) {
			ImGui::Checkbox("Bloom", &bloom_);
			if (bloom_) {

				if (ImGui::Button("Reload Bloom Shader")) {
					bloomEffect_.reload();
				}
				ImGui::SliderFloat("intensity", &bloomEffect_.intensity_, 0.f, 1.f);
				ImGui::SliderFloat("exposure", &bloomEffect_.exposure_, 0.f, 10.f);
				ImGui::Checkbox("High contrast", &bloomEffect_.highContrast_);
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Env Settings")) {
			renderSceneTab();
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

void BHVApp::renderSceneTab()
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
		if(currentEnvironmentScene_)
			currentEnvironmentScene_->renderGui();
	}
}

void BHVApp::dumpState(std::string const& file) {
	boost::json::object configuration;
	configuration["camOrbit"] = camOrbit_;
	configuration["camOrbitTilt"] = camOrbitTilt_;
	configuration["camOrbitRad"] = camOrbitRad_;
	configuration["camOrbitSpeed"] = camOrbitSpeed_;
	configuration["camOrbitAngle"] = camOrbitAngle_;
	configuration["aberration"] = aberration_;
	configuration["fido"] = fido_;
	configuration["useCustomDirection"] = useCustomDirection_;
	configuration["useLocalDirection"] = useLocalDirection_;
	configuration["speed"] = speed_;
	configuration["fboScale"] = fboScale_;
	configuration["bloom"] = bloom_;
	configuration["direction"] = { direction_.x, direction_.y, direction_.z };

	boost::json::object discConfiguration;
	disc_->storeConfig(discConfiguration);
	configuration["disc"] = discConfiguration;

	boost::json::object shaderConfiguration;
	shaderElement_->storeConfig(shaderConfiguration);
	configuration["shader"] = shaderConfiguration;

	boost::json::object bloomConfiguration;
	bloomEffect_.storeConfig(bloomConfiguration);
	configuration["bloomEffect"] = bloomConfiguration;

	boost::json::object cameraConfiguration;
	cam_.storeConfig(cameraConfiguration);
	configuration["camera"] = cameraConfiguration;

	std::string json = boost::json::serialize(configuration);
	std::ofstream outFile(ROOT_DIR "saves/" + file);
	outFile << json;
	outFile.close();
	return;
}

void BHVApp::readState(std::string const& file) {

	if (!std::filesystem::exists(ROOT_DIR "saves/" + file)) {
		std::cerr << "[BHVApp] file " << ROOT_DIR "saves/" + file << " not found" << std::endl;
		return;
	}
	std::ifstream inFile(ROOT_DIR "saves/" + file);
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
	jhelper::getValue(configuration, "camOrbit", camOrbit_);
	jhelper::getValue(configuration, "camOrbitTilt", camOrbitTilt_);
	jhelper::getValue(configuration, "camOrbitRad", camOrbitRad_);
	jhelper::getValue(configuration, "camOrbitSpeed", camOrbitSpeed_);
	jhelper::getValue(configuration, "camOrbitAngle", camOrbitAngle_);
	jhelper::getValue(configuration, "aberration", aberration_);
	jhelper::getValue(configuration, "fido", fido_);
	jhelper::getValue(configuration, "useCustomDirection", useCustomDirection_);
	jhelper::getValue(configuration, "useLocalDirection", useLocalDirection_);
	jhelper::getValue(configuration, "speed", speed_);
	jhelper::getValue(configuration, "fboScale", fboScale_);
	jhelper::getValue(configuration, "bloom", bloom_);
	jhelper::getValue(configuration, "direction", direction_);

	boost::json::object discConfiguration;
	if(jhelper::getValue(configuration, "disc", discConfiguration))
		disc_->loadConfig(discConfiguration);

	boost::json::object shaderConfiguration;
	if(jhelper::getValue(configuration, "shader",shaderConfiguration))
		shaderElement_->loadConfig(shaderConfiguration);

	boost::json::object bloomConfiguration;
	if(jhelper::getValue(configuration, "bloomEffect", bloomConfiguration))
		bloomEffect_.loadConfig(bloomConfiguration);

	boost::json::object cameraConfiguration;
	if(jhelper::getValue(configuration, "camera", cameraConfiguration))
		cam_.storeConfig(cameraConfiguration);

	resizeTextures();
	
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
	float speed = 648.857971f;
	glm::vec3 dir(-0.147305459f, -0.722951770, -0.675012469);
	glm::mat4 boost = cam_.getBoostFromVel(dir, speed);

	std::cout << glm::to_string(boost) << std::endl;

}