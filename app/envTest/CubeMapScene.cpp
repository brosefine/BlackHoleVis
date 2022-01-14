#include "CubeMapScene.h"
#include <glm/gtx/transform.hpp>

CubeMapScene::CubeMapScene() : size_(0)
{
}

CubeMapScene::CubeMapScene(int size) : size_(size)
{
	initCameras();
	initEnvMap();
}

CubeMapScene::~CubeMapScene()
{
	glDeleteFramebuffers(1, &fboID_);
}

void CubeMapScene::initCameras()
{
	glm::vec3 camPos = glm::vec3(0.f);
	envCameras_.push_back(std::make_shared<SimpleCamera>(camPos, glm::vec3(0.0, -1.0, 0.0), glm::vec3(1.0, 0.0, 0.0), 90.f));
	envCameras_.push_back(std::make_shared<SimpleCamera>(camPos, glm::vec3(0.0, -1.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), 90.f));
	envCameras_.push_back(std::make_shared<SimpleCamera>(camPos, glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 1.0, 0.0), 90.f));
	envCameras_.push_back(std::make_shared<SimpleCamera>(camPos, glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0), 90.f));
	envCameras_.push_back(std::make_shared<SimpleCamera>(camPos, glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), 90.f));
	envCameras_.push_back(std::make_shared<SimpleCamera>(camPos, glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0), 90.f));
}


void CubeMapScene::initEnvMap() {
	// init empty Cubemap
	envMap_ = std::make_shared<CubeMap>(size_, size_);
	glTextureStorage2D(envMap_->getTexId(), 1, GL_RGB32F,
		size_, size_);
	envMap_->generateMipMap();
	std::vector<std::pair<GLenum, GLint>> texParametersi = {
	{GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
	{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
	{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
	{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
	{GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE}
	};
	envMap_->setParam(texParametersi);
	envMap_->setParam(GL_TEXTURE_MAX_ANISOTROPY, 10.0f);

	// init empty Depth Cubemap
	depthMap_ = std::make_shared<CubeMap>(size_, size_);
	glTextureStorage2D(depthMap_->getTexId(), 1, GL_DEPTH24_STENCIL8,
		size_, size_);
	depthMap_->generateMipMap();
	texParametersi = {
	{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
	{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
	{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
	{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
	{GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE}
	};
	depthMap_->setParam(texParametersi);
	// create FBO
	glGenFramebuffers(1, &fboID_);
	glBindFramebuffer(GL_FRAMEBUFFER, fboID_);

	//bind first cube face to fbo
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, envMap_->getTexId(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, depthMap_->getTexId(), 0);

	if (!glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "sky fbo not complete" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CubeMapScene::updateCameras(glm::vec3 camPos)
{
	for (auto& cam : envCameras_)
		cam->setPos(camPos);
}

SolarSystemScene::SolarSystemScene(): CubeMapScene()
{
}

SolarSystemScene::SolarSystemScene(int size): CubeMapScene(size)
{
}

void SolarSystemScene::render(glm::vec3 camPos, float dt)
{
	updateCameras(camPos);
	glBindFramebuffer(GL_FRAMEBUFFER, fboID_);
	glViewport(0, 0, envMap_->getWidth(), envMap_->getHeight());

	for (unsigned int i = 0; i < 6; ++i) {

		GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face, envMap_->getTexId(), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, face, envMap_->getTexId(), 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		envCameras_.at(i)->use(envMap_->getWidth(), envMap_->getHeight());

		glActiveTexture(GL_TEXTURE0);
		meshTexture_->bind();

		meshShader_->use();

		static float rotAngle = 0;
		rotAngle += speedScale_ * dt_;
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
}
