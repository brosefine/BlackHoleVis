#pragma once

#include <gui/guiElement.h>
#include <objects/accretionDisk.h>
#include <helpers/uboBindings.h>
#include <helpers/json_helper.h>

#include <boost/json.hpp>
#include <glm/gtc/type_ptr.hpp>



class BlackHoleShaderGui : public ShaderGui {
public:

	BlackHoleShaderGui()
		: starExposure_(1.f) {
		name_ = "Black Hole";
		shader_ = std::make_shared<Shader>(
			std::vector<std::string>{ "kerr/render.vert" }, 
			std::vector<std::string>{"kerr/render.frag"},
			std::vector<std::string>{"PINHOLE", "STARS", "DOME", "MWPANORAMA", "DEFLECTIONMAP", "LINEARSAMPLE"});
		preprocessorFlags_ = shader_->getFlags();
		shader_->use();
		shader_->setBlockBinding("camera", CAMBINDING);
	}

private:
	
	float starExposure_;

	void render() override {
		ImGui::Text("I'm the Black Hole Shader");
		ImGui::SliderFloat("Star Exposure", &starExposure_, 0.f, 1.f);
		renderPreprocessorFlags();
		ImGui::Separator();

		renderRefreshMenu();
	}

	void uploadUniforms() override {
		shader_->setUniform("star_exposure", starExposure_);
	}

};

class QuadShaderGui : public ShaderGui {
public:

	QuadShaderGui()
		: gamma_(2.2f)
		, exposure_(1.f)
		, bloom_(false)
		, tonemap_(false)	{
		name_ = "Quad";
		shader_ = std::make_shared<Shader>("squad.vs", "squad.fs");
		preprocessorFlags_ = shader_->getFlags();
		shader_->use();
		uploadUniforms();
	}

	void storeConfig(boost::json::object& obj) override {
		obj["tonemap"] = tonemap_;
		obj["bloom"] = bloom_;
		obj["exposure"] = exposure_;
		obj["gamma"] = gamma_;
		storePreprocessorFlags(obj);
	}

	void loadConfig(boost::json::object& obj) override {
		jhelper::getValue(obj, "tonemap", tonemap_);
		jhelper::getValue(obj, "bloom", bloom_);
		jhelper::getValue(obj, "exposure", exposure_);
		jhelper::getValue(obj, "gamma", gamma_);
		loadPreprocessorFlags(obj);
		update();
	}
private:

	float gamma_;
	float exposure_;
	bool bloom_;
	bool tonemap_;

	void render() override {
		ImGui::Text("Post Processing Settings");
		ImGui::SliderFloat("Exposure", &exposure_, 0.f, 2.f);
		ImGui::SliderFloat("Gamma", &gamma_, 0.f, 5.f);
		ImGui::Checkbox("Tone Mapping", &tonemap_);
		renderRefreshMenu();
	}

	void uploadUniforms() override {
		shader_->setUniform("tonemap", tonemap_);
		shader_->setUniform("exposure", exposure_);
		shader_->setUniform("gamma", gamma_);
	}


};
