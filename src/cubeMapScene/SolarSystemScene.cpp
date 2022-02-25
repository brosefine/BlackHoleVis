#include <cubeMapScene/SolarSystemScene.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <helpers/uboBindings.h>
#include <gui/gui.h>


SolarSystemScene::SolarSystemScene()
	: CubeMapScene()
	, distScale_(10.f)
	, sizeScale_(1.f)
	, rotationSpeedScale_(0.f)
{}

SolarSystemScene::SolarSystemScene(int size)
	: CubeMapScene(size)
	, sphereMesh_(std::make_shared<Mesh>("sphere.obj"))
	, meshTextures_()
	, planets_()
	, modelMatrices_()
	, distScale_(4.f)
	, sizeScale_(0.1f)
	, maxSize_(20.f)
	, rotationSpeedScale_(0.01f)
{
	initModelTransforms();
	initPlanets();
	loadShaders();
	loadTextures();

	std::vector<std::pair<GLenum, GLint>> texParametersi = {
	{GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
	{GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR},
	{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
	{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
	{GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE}
	};
	envMap_->generateMipMap();
	envMap_->setParam(texParametersi);
	envMap_->setParam(GL_TEXTURE_MAX_ANISOTROPY, 16.f);
}

void SolarSystemScene::render(glm::vec3 camPos, float dt)
{
	updateCameras(camPos);
	updateModelTransforms(dt);

	glBindFramebuffer(GL_FRAMEBUFFER, fboID_);
	glViewport(0, 0, envMap_->getWidth(), envMap_->getHeight());

	glEnable(GL_DEPTH_TEST);
	for (unsigned int i = 0; i < 6; ++i) {

		GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face, envMap_->getTexId(), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, face, depthMap_->getTexId(), 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		envCameras_.at(i)->use(envMap_->getWidth(), envMap_->getHeight());

		for (auto const& [planetName, planetTransform] : modelMatrices_) {

			meshShader_->use();
			meshShader_->setUniform("modelMatrix", planetTransform);
			
			glActiveTexture(GL_TEXTURE0);
			bool useTexture = false;
			if (meshTextures_.contains(planetName)) {
				meshTextures_[planetName]->bind();
				useTexture = true;
			}
			else if (planets_.contains(planetName)) {
				meshShader_->setUniform("mesh_color", planets_[planetName].color_);
			}
			else {
				meshShader_->setUniform("mesh_color", glm::vec3(1.f));
			}
			meshShader_->setUniform("use_texture", useTexture);

			sphereMesh_->draw(GL_TRIANGLES);

		}

		
		glActiveTexture(GL_TEXTURE0);
		skyTexture_->bind();

		skyShader_->use();
		quad_.draw(GL_TRIANGLES);
		

	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);

	envMap_->generateMipMap();

}

void SolarSystemScene::renderGui()
{
	ImGui::Text("Solar System Scene Settings");
	ImGui::SliderFloat("Distance Scale", &distScale_, 0.f, 20.f);
	ImGui::SliderFloat("Size Scale", &sizeScale_, 0.f, 1.f);
	ImGui::SliderFloat("Max Size (relative to Earth)", &maxSize_, 0.f, 20.f);
	ImGui::SliderFloat("Rotation Speed Scale", &rotationSpeedScale_, 0.f, 1.f);
	if (ImGui::Button("Reload Shaders"))
		reloadShaders();
	ImGui::Separator();
	ImGui::Text("Object Colors");
	for (auto& [planetName, planetProps] : planets_) {
		ImGui::SliderFloat3(objToString(planetName).c_str(), glm::value_ptr(planetProps.color_), 0.f, 1.f);
	}
	ImGui::Separator();
}

void SolarSystemScene::loadTextures()
{
	meshTextures_[Objects::MERCURY] = std::make_shared<Texture2D>("planets/mercury.jpg");
	meshTextures_[Objects::VENUS] = std::make_shared<Texture2D>("planets/venus.jpg");
	meshTextures_[Objects::EARTH] = std::make_shared<Texture2D>("planets/earth.jpg");
	meshTextures_[Objects::MOON] = std::make_shared<Texture2D>("planets/moon.jpg");
	meshTextures_[Objects::MARS] = std::make_shared<Texture2D>("planets/mars.jpg");
	meshTextures_[Objects::JUPITER] = std::make_shared<Texture2D>("planets/jupiter.jpg");
	meshTextures_[Objects::SATURN] = std::make_shared<Texture2D>("planets/saturn.jpg");
	meshTextures_[Objects::URANUS] = std::make_shared<Texture2D>("planets/uranus.jpg");
	meshTextures_[Objects::NEPTUNE] = std::make_shared<Texture2D>("planets/neptune.jpg");

	std::vector<std::pair<GLenum, GLint>> texParameters{
		{GL_TEXTURE_WRAP_S, GL_REPEAT},
		{GL_TEXTURE_WRAP_T, GL_REPEAT},
		{GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
		{GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR}
	};
	for (auto& [obj, tex] : meshTextures_) {
		tex->setParam(texParameters);
		tex->generateMipMap();
	}

	std::vector<std::string> skyFaces = { "milkyway2048/right.png", "milkyway2048/left.png", "milkyway2048/top.png", "milkyway2048/bottom.png", "milkyway2048/front.png", "milkyway2048/back.png" };
	skyTexture_ = std::make_shared<CubeMap>(skyFaces);
}

void SolarSystemScene::loadShaders()
{
	skyShader_ = std::make_shared<Shader>("cubeMapScene/sky.vs", "cubeMapScene/sky.fs");
	meshShader_ = std::make_shared<Shader>("cubeMapScene/mesh.vs", "cubeMapScene/mesh.fs");
	reloadShaders();
}

void SolarSystemScene::initModelTransforms()
{
	modelMatrices_.insert({ Objects::MERCURY, glm::mat4(1) });
	modelMatrices_.insert({ Objects::VENUS, glm::mat4(1) });
	modelMatrices_.insert({ Objects::EARTH, glm::mat4(1) });
	modelMatrices_.insert({ Objects::MOON, glm::mat4(1) });
	modelMatrices_.insert({ Objects::MARS, glm::mat4(1) });
	modelMatrices_.insert({ Objects::JUPITER, glm::mat4(1) });
	modelMatrices_.insert({ Objects::SATURN, glm::mat4(1) });
	modelMatrices_.insert({ Objects::URANUS, glm::mat4(1) });
	modelMatrices_.insert({ Objects::NEPTUNE, glm::mat4(1) });
}

void SolarSystemScene::initPlanets()
{
#pragma region mercury
	PlanetProperties mercuryProps;
	mercuryProps.dist_ = 0.387f; mercuryProps.size_ = 0.766f;
	mercuryProps.rotSpeed_ = 356.f / 176.f; mercuryProps.orbitSpeed_ = 365.f / 87.f;
	mercuryProps.inclination_ = glm::radians(7.f); mercuryProps.axisTilt_ = 0.f;
	mercuryProps.color_ = glm::vec3(0.75f);
	planets_.insert({ Objects::MERCURY,  mercuryProps });
#pragma endregion

#pragma region venus
	PlanetProperties venusProps;
	venusProps.dist_ = 0.728f; venusProps.size_ = 0.949f;
	venusProps.rotSpeed_ = 356.f / -116.f; venusProps.orbitSpeed_ = 365.f / 224.f;
	venusProps.inclination_ = glm::radians(3.3f); venusProps.axisTilt_ = glm::radians(2.64f);
	venusProps.color_ = glm::vec3(0.88f, 0.66f, 0.39f);
	planets_.insert({ Objects::VENUS,  venusProps });
#pragma endregion

#pragma region earth
	PlanetProperties earthProps;
	earthProps.dist_ = 1.f; earthProps.size_ = 1.f;
	earthProps.rotSpeed_ = 1.f; earthProps.orbitSpeed_ = 1.f;
	earthProps.inclination_ = 0.f; earthProps.axisTilt_ = glm::radians(23.4f);
	earthProps.color_ = glm::vec3(0.13f, 0.18f, 0.38f);
	planets_.insert({ Objects::EARTH,  earthProps});
#pragma endregion

#pragma region moon
	PlanetProperties moonProps;
	moonProps.dist_ = 1.1f; moonProps.size_ = 0.272f;
	moonProps.rotSpeed_ = 356.f / 29.f; moonProps.orbitSpeed_ = 365.f / 29.f;
	moonProps.inclination_ = glm::radians(5.1f); moonProps.axisTilt_ = glm::radians(6.8f);
	moonProps.color_ = glm::vec3(0.5f);
	planets_.insert({ Objects::MOON,  moonProps });
#pragma endregion

#pragma region mars
	PlanetProperties marsProps;
	marsProps.dist_ = 1.523f; marsProps.size_ = 0.532f;
	marsProps.rotSpeed_ = 1.f; marsProps.orbitSpeed_ = 365.f / 686.f;
	marsProps.inclination_ = glm::radians(1.8f); marsProps.axisTilt_ = glm::radians(25.f);
	marsProps.color_ = glm::vec3(0.55f, 0.17f, 0.1);
	planets_.insert({ Objects::MARS,  marsProps });
#pragma endregion

#pragma region jupiter
	PlanetProperties jupiterProps;
	jupiterProps.dist_ = 5.2f; jupiterProps.size_ = 10.973f;
	jupiterProps.rotSpeed_ = 356.f / 20.f /*0.37f*/; jupiterProps.orbitSpeed_ = 365.f / 4332.f;
	jupiterProps.inclination_ = glm::radians(1.3f); jupiterProps.axisTilt_ = glm::radians(3.13f);
	jupiterProps.color_ = glm::vec3(0.88f, 0.83f, 0.6f);
	planets_.insert({ Objects::JUPITER,  jupiterProps });
#pragma endregion

#pragma region saturn
	PlanetProperties saturnProps;
	saturnProps.dist_ = 9.579f; saturnProps.size_ = 9.14f;
	saturnProps.rotSpeed_ = 356.f / 30.f /*0.41f*/; saturnProps.orbitSpeed_ = 365.f / 10759.f;
	saturnProps.inclination_ = glm::radians(2.5f); saturnProps.axisTilt_ = glm::radians(26.7f);
	saturnProps.color_ = glm::vec3(0.56f, 0.44f, 0.25f);
	planets_.insert({ Objects::SATURN,  saturnProps });
#pragma endregion

#pragma region uranus
	PlanetProperties uranusProps;
	uranusProps.dist_ = 19.185f; uranusProps.size_ = 3.98f;
	uranusProps.rotSpeed_ = 356.f / -0.7f; uranusProps.orbitSpeed_ = 365.f / 30688.f;
	uranusProps.inclination_ = glm::radians(0.7f); uranusProps.axisTilt_ = glm::radians(90.f);
	uranusProps.color_ = glm::vec3(0.45f, 0.57f, 0.59f);
	planets_.insert({ Objects::URANUS,  uranusProps });
#pragma endregion

#pragma region neptune
	PlanetProperties neptuneProps;
	neptuneProps.dist_ = 30.08f; neptuneProps.size_ = 3.864f;
	neptuneProps.rotSpeed_ = 356.f / 0.67f; neptuneProps.orbitSpeed_ = 365.f / 60195.f;
	neptuneProps.inclination_ = glm::radians(1.7f); neptuneProps.axisTilt_ = glm::radians(28.32f);
	neptuneProps.color_ = glm::vec3(0.46f, 0.34f, 0.68f);
	planets_.insert({ Objects::NEPTUNE,  neptuneProps });
#pragma endregion
	
}

void SolarSystemScene::updateModelTransforms(float dt)
{
	static float rotAngle = 0;
	rotAngle += rotationSpeedScale_ * dt;

	for (auto const& [name, props] : planets_) {
		if (name == Objects::MOON) continue;
		modelMatrices_[name] =
			glm::rotate((props.inclination_), glm::vec3(0, 0, 1)) *
			glm::rotate((props.orbitSpeed_ * rotAngle), glm::vec3(0, 1, 0)) *
			glm::translate(glm::vec3(props.dist_ * distScale_, 0, 0)) *
			glm::rotate((props.axisTilt_), glm::vec3(1, 0, 0)) *
			glm::rotate((props.rotSpeed_ * rotAngle), glm::vec3(0, 1, 0)) *
			glm::scale(glm::vec3(glm::min(maxSize_, props.size_) * sizeScale_)) *
			glm::mat4(1);
	}

	PlanetProperties earthProps = planets_[Objects::EARTH];
	PlanetProperties moonProps = planets_[Objects::MOON];
	modelMatrices_[Objects::MOON] =
		glm::rotate((earthProps.orbitSpeed_ * rotAngle), glm::vec3(0, 1, 0)) *
		glm::translate(glm::vec3(earthProps.dist_ * distScale_, 0, 0)) *
		glm::rotate((moonProps.inclination_), glm::vec3(0, 0, 1)) *
		glm::rotate((moonProps.orbitSpeed_ * rotAngle), glm::vec3(0, 1, 0)) *
		glm::translate(glm::vec3(moonProps.dist_ * distScale_ * earthProps.size_ * sizeScale_, 0, 0)) *
		glm::rotate((moonProps.axisTilt_), glm::vec3(1, 0, 0)) *
		glm::rotate((moonProps.rotSpeed_ * rotAngle), glm::vec3(0, 1, 0)) *
		glm::scale(glm::vec3(glm::min(maxSize_, moonProps.size_) * sizeScale_)) *
		glm::mat4(1);;

}


void SolarSystemScene::reloadShaders()
{
	meshShader_->reload();
	skyShader_->reload();
	skyShader_->setBlockBinding("camera", CAMBINDING);
	meshShader_->setBlockBinding("camera", CAMBINDING);
}

std::string SolarSystemScene::objToString(Objects obj)
{
	switch (obj)
	{
	case Objects::MERCURY:
		return "Mercury";
	case Objects::VENUS:
		return "Venus";
	case Objects::EARTH:
		return "Earth";
	case Objects::MOON:
		return "Moon";
	case Objects::MARS:
		return "Mars";
	case Objects::JUPITER:
		return "Jupiter";
	case Objects::SATURN:
		return "Saturn";
	case Objects::URANUS:
		return "Uranus";
	case Objects::NEPTUNE:
		return "Neptune";
	default:
		return "Unknown";
	}
}
