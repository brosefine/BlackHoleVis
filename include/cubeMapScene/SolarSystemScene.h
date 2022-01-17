#pragma once
#include <cubeMapScene/CubeMapScene.h>

#include <map>

#include <rendering/mesh.h>
#include <rendering/shader.h>

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
	float sceneScale_;

	void loadTextures();
	void loadShaders();
	void initModelTransforms();
	void initColors();
	void updateModelTransforms(float dt);
	void reloadShaders();
	std::string objToString(Objects obj);

};