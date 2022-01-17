#include <cubeMapScene/CubeMapScene.h>

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

	// init empty Depth Cubemap
	depthMap_ = std::make_shared<CubeMap>(size_, size_);
	glTextureStorage2D(depthMap_->getTexId(), 1, GL_DEPTH24_STENCIL8,
		size_, size_);
	
	std::vector<std::pair<GLenum, GLint>> texParametersi = {
	{GL_TEXTURE_MIN_FILTER, GL_LINEAR},
	{GL_TEXTURE_MAG_FILTER, GL_LINEAR},
	{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
	{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
	{GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE}
	};
	envMap_->setParam(texParametersi);
	depthMap_->setParam(texParametersi);

	// create FBO
	glGenFramebuffers(1, &fboID_);
	glBindFramebuffer(GL_FRAMEBUFFER, fboID_);

	//bind first cube face to fbo
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, envMap_->getTexId(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, depthMap_->getTexId(), 0);

	if (!glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "[Solar System Scene] sky fbo not complete" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CubeMapScene::updateCameras(glm::vec3 camPos)
{
	for (auto& cam : envCameras_)
		cam->setPos(camPos);
}
