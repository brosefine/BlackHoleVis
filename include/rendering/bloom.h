#pragma  once
#include <rendering/window.h>
#include <rendering/mesh.h>
#include <rendering/shader.h>
#include <rendering/texture.h>

class Bloom {
public:
	Bloom(){}
	Bloom(int w, int h, int level = 9);

	void begin();
	void render(int fboId = 0);
	void reload();
	void resize(int width, int height);

	float intensity_;
	float exposure_;
	bool highContrast_;

private:
	int maxLevels_;
	int width_;
	int height_;

	std::vector<glm::vec2> levelSizes_;

	Quad quad_;

	std::shared_ptr<ShaderBase> upsampleShader_;
	std::shared_ptr<ShaderBase> bloomShader_;
	std::shared_ptr<ShaderBase> renderShader_;

	std::shared_ptr<FBOTexture> source_;
	std::shared_ptr<FBOTexture> filters_;

	void initShaders();
	void initFBOS();

};