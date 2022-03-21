#pragma once

#include <cubeMapScene/CubeMapScene.h>
#include <rendering/mesh.h>
#include <rendering/shader.h>

class CheckerSphereScene : public CubeMapScene {
public:

	CheckerSphereScene();
	CheckerSphereScene(int size);

	void render(glm::vec3 camPos, float dt) override;
	void renderGui() override;

private:

	Quad quad_;
	std::shared_ptr<Mesh> sphereMesh_;
	std::shared_ptr<Mesh> cubeMesh_;
	std::shared_ptr<CubeMap> skyTexture_;

	std::shared_ptr<ShaderBase> sphereShader_;
	std::shared_ptr<ShaderBase> cubeShader_;
	std::shared_ptr<ShaderBase> skyShader_;

	glm::vec3 sphereColor_;
	glm::vec3 spherePos_, cubePos_;
	float sphereScale_, cubeScale_;
	bool drawSky_;

	void loadTextures();
	void loadShaders();
	void reloadShaders();
};