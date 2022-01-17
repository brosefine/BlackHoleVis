#pragma once

#include <memory>
#include <map>

#include <glm/glm.hpp>
#include <rendering/window.h>
#include <rendering/simpleCamera.h>
#include <rendering/texture.h>
#include <rendering/mesh.h>
#include <rendering/shader.h>




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

class SolarSystemScene : public CubeMapScene {
public:

	enum class Objects {
		EARTH,
		MOON, 
		MARS
	};

	SolarSystemScene();
	SolarSystemScene(int size);

	void render(glm::vec3 camPos, float dt) override;
	void renderGui() override;

private:

	Quad quad_;
	std::shared_ptr<Mesh> sphereMesh_;
	std::shared_ptr<CubeMap> skyTexture_;
	std::map<Objects, std::shared_ptr<Texture2D>> meshTextures_;
	std::map<Objects, glm::vec3> meshColors_;
	std::map<Objects, glm::mat4> modelMatrices_;

	std::shared_ptr<ShaderBase> meshShader_;
	std::shared_ptr<ShaderBase> skyShader_;

	float rotationSpeedScale_;
	glm::vec2 rotationTilt_;

	void loadTextures();
	void loadShaders();
	void initModelTransforms();
	void initColors();
	void updateModelTransforms(float dt);
	void reloadShaders();
	std::string objToString(Objects obj);

};