#include <cubeMapScene/SolarSystemScene.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <helpers/uboBindings.h>
#include <gui/gui.h>


SolarSystemScene::SolarSystemScene()
	: CubeMapScene()
	, sceneScale_(1.f)
	, rotationSpeedScale_(0.f)
	, rotationTilt_(0.f)
{}

SolarSystemScene::SolarSystemScene(int size)
	: CubeMapScene(size)
	, sphereMesh_(std::make_shared<Mesh>("sphere.obj"))
	, meshTextures_()
	, meshColors_()
	, modelMatrices_()
	, sceneScale_(1.f)
	, rotationSpeedScale_(1.f)
	, rotationTilt_(-0.4f)
{
	initModelTransforms();
	initColors();
	loadShaders();
	loadTextures();
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

		for (auto const& [objName, objTransform] : modelMatrices_) {

			meshShader_->use();
			meshShader_->setUniform("modelMatrix", glm::scale(glm::vec3(sceneScale_)) * objTransform);
			
			glActiveTexture(GL_TEXTURE0);
			bool useTexture = false;
			if (meshTextures_.contains(objName)) {
				meshTextures_[objName]->bind();
				useTexture = true;
			}
			else if (meshColors_.contains(objName)) {
				meshShader_->setUniform("mesh_color", meshColors_[objName]);
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

}

void SolarSystemScene::renderGui()
{
	ImGui::Text("Solar System Scene Settings");
	ImGui::SliderFloat("Scene Scale", &sceneScale_, 0.f, 10.f);
	ImGui::SliderFloat("Rotation Scale", &rotationSpeedScale_, 0.f, 10.f);
	ImGui::SliderFloat2("Rotation Tilt X,Z", glm::value_ptr(rotationTilt_), -1.f, 1.f);
	if (ImGui::Button("Reload Shaders"))
		reloadShaders();
	ImGui::Separator();
	ImGui::Text("Object Colors");
	for (auto& [objName, objColor] : meshColors_) {
		ImGui::SliderFloat3(objToString(objName).c_str(), glm::value_ptr(objColor), 0.f, 1.f);
	}
	ImGui::Separator();
}

void SolarSystemScene::loadTextures()
{
	meshTextures_[Objects::EARTH] =  std::make_shared<Texture2D>("planets/Earthmap.jpg");
	meshTextures_[Objects::MARS] =  std::make_shared<Texture2D>("planets/mars.jpg");

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
	modelMatrices_.insert({ Objects::EARTH, glm::mat4(1) });
	modelMatrices_.insert({ Objects::MARS, glm::mat4(1) });
	modelMatrices_.insert({ Objects::MOON, glm::mat4(1) });
}

void SolarSystemScene::initColors()
{
	meshColors_.insert({ Objects::EARTH, glm::vec3(0.f, 0.1f, 1.f) });
	meshColors_.insert({ Objects::MARS, glm::vec3(0.7f, 0.3f, 0.1f) });
	meshColors_.insert({ Objects::MOON, glm::vec3(0.5) });
}

void SolarSystemScene::updateModelTransforms(float dt)
{
	static float rotAngle = 0;
	rotAngle += rotationSpeedScale_ * dt;

	// Earth
	const float earthDist = 10.f, earthScale = 1.f, earthRotSpeed = 1.f, earthOrbitSpeed = 1.f;
	modelMatrices_[Objects::EARTH] = 
		glm::rotate(glm::radians(earthOrbitSpeed * rotAngle), glm::vec3(0, 1, 0)) * 
		glm::translate(glm::vec3(earthDist, 0, 0)) * 
		glm::rotate(glm::radians(earthRotSpeed * rotAngle), glm::vec3(rotationTilt_.x, 1, rotationTilt_.y)) * 
		glm::mat4(1);

	// Moon
	const float moonDist = 3.f, moonScale = 0.2f, moonRotSpeed = 1.5f, moonOrbitSpeed = 1.5f;
	modelMatrices_[Objects::MOON] = 
		modelMatrices_[Objects::EARTH] * 
		glm::rotate(glm::radians(moonOrbitSpeed * rotAngle), glm::vec3(0, 1, 0)) * 
		glm::translate(glm::vec3(moonDist, 0, 0)) * 
		glm::scale(glm::vec3(moonScale)) * 
		glm::rotate(glm::radians(moonRotSpeed * rotAngle), glm::vec3(rotationTilt_.x, 1, rotationTilt_.y)) * 
		glm::mat4(1);

	// Mars
	const float marsDist = 20.f, marsScale = 0.8f, marsRotSpeed = 0.9f, marsOrbitSpeed = 0.7f;
	modelMatrices_[Objects::MARS] = 
		glm::rotate(glm::radians(marsOrbitSpeed * rotAngle), glm::vec3(0, 1, 0)) * 
		glm::translate(glm::vec3(0, 0, -marsDist))  * 
		glm::scale(glm::vec3(marsScale))  * 
		glm::rotate(glm::radians(marsRotSpeed * rotAngle), glm::vec3(rotationTilt_.x, 1, rotationTilt_.y)) * 
		glm::mat4(1);

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
	case SolarSystemScene::Objects::EARTH:
		return "Earth";
	case SolarSystemScene::Objects::MOON:
		return "Moon";
	case SolarSystemScene::Objects::MARS:
		return "Mars";
	default:
		return "Unknown";
	}
}
