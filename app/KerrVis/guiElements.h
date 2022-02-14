#pragma once

#include <gui/guiElement.h>
#include <objects/accretionDisk.h>
#include <helpers/uboBindings.h>
#include <helpers/json_helper.h>

#include <boost/json.hpp>
#include <glm/gtc/type_ptr.hpp>



class BlackHoleShaderGui : public ShaderGui {
public:

	BlackHoleShaderGui(){
		name_ = "Black Hole";
		shader_ = std::make_shared<Shader>(
			std::vector<std::string>{ "kerr/render.vert" }, 
			std::vector<std::string>{"kerr/render.frag"},
			std::vector<std::string>{"PINHOLE", "STARS", "DOME"});
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
