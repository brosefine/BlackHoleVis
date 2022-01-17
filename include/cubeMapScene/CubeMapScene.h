#pragma once

#include <memory>
#include <iostream>
#include <glm/glm.hpp>

#include <rendering/texture.h>
#include <rendering/window.h>
#include <rendering/simpleCamera.h>


class CubeMapScene {
public:
	CubeMapScene();
	CubeMapScene(int size);
	~CubeMapScene();

	virtual void render(glm::vec3 camPos, float dt) = 0;
	virtual void renderGui() = 0;

	void bindEnv() const { envMap_->bind(); }
	void bindDepth() const { depthMap_->bind(); }
	GLuint getEnvId() const { return envMap_->getTexId(); }
	GLuint getDepthId() const { return depthMap_->getTexId(); }

protected:

	int size_;
	GLuint fboID_;

	std::shared_ptr<CubeMap> envMap_;
	std::shared_ptr<CubeMap> depthMap_;
	std::vector<std::shared_ptr<SimpleCamera>> envCameras_;

	void initCameras();
	void initEnvMap();
	void updateCameras(glm::vec3 camPos);
};