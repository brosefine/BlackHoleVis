#pragma once

#include <gui/guiElement.h>

class BlackHoleShaderGui : public ShaderGui {
public:

	BlackHoleShaderGui(): pinhole_(true) {

		name_ = "Black Hole";
		shader_ = std::make_shared<Shader>(
			std::vector<std::string>{ "black_hole_shader.vert" }, 
			std::vector<std::string>{ "black_hole_shader_color.frag", "black_hole_shader_main.frag"},
			std::vector<std::string>{"DISK"});
		preprocessorFlags_ = shader_->getFlags();
		shader_->use();
		shader_->setBlockBinding("camera", CAMBINDING);
	}

private:

	bool pinhole_;

	void uploadUniforms() override {
			shader_->setUniform("pinhole", pinhole_);
	}

	void render() override {
		ImGui::Text("I'm the Black Hole Shader");
		if(ImGui::Checkbox("Pinhole View", &pinhole_))
			shader_->setUniform("pinhole", pinhole_);

		renderPreprocessorFlags();
		ImGui::Separator();

		renderRefreshMenu();
	}
};
