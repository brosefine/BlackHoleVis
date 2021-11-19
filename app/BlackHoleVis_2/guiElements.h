#pragma once

#include <gui/guiElement.h>

class BlackHoleShaderGui : public ShaderGui {
public:

	BlackHoleShaderGui() {

		name_ = "Black Hole";
		shader_ = std::shared_ptr<ShaderBase>(new Shader("black_hole_shader.vert", "black_hole_shader.frag"));
		preprocessorFlags_ = shader_->getFlags();
		shader_->use();
		shader_->setBlockBinding("camera", CAMBINDING);
	}

private:
	void render() override {
		ImGui::Text("I'm the Black Hole Shader");
		renderPreprocessorFlags();
		ImGui::Separator();

		renderRefreshMenu();
	}
};
