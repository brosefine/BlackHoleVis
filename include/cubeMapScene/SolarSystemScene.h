#pragma once
#include <cubeMapScene/CubeMapScene.h>

#include <map>

#include <rendering/mesh.h>
#include <rendering/shader.h>

class SolarSystemScene : public CubeMapScene {
public:

	enum class Objects {
		MERCURY,
		VENUS,
		EARTH,
		MOON, 
		MARS,
		JUPITER,
		SATURN,
		URANUS,
		NEPTUNE
	};

	struct PlanetProperties {
		float dist_;
		float size_;
		float rotSpeed_;
		float orbitSpeed_;
		float inclination_;
		float axisTilt_;
		glm::vec3 color_;
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
	std::map<Objects, PlanetProperties> planets_;
	std::map<Objects, glm::mat4> modelMatrices_;

	std::shared_ptr<ShaderBase> meshShader_;
	std::shared_ptr<ShaderBase> skyShader_;

	float rotationSpeedScale_;
	float distScale_;
	float sizeScale_;
	float maxSize_;

	void loadTextures();
	void loadShaders();
	void initModelTransforms();
	void initPlanets();
	void updateModelTransforms(float dt);
	void reloadShaders();
	std::string objToString(Objects obj);

};