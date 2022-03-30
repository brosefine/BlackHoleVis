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
		: maxBrightness_(2.f)
		, jetAngle_(0.4f)
		, jetSize_(3.f, 9.f)
		, scale_(1.f)
		, starExposure_(1.f){

		name_ = "Black Hole";
		shader_ = std::make_shared<Shader>(
			std::vector<std::string>{ "ebruneton/black_hole_shader.vert" }, 
			std::vector<std::string>{"ebruneton/black_hole_shader.frag"},
			std::vector<std::string>{"DISC", "DOPPLER", "PINHOLE", "STARS", "JET", "DOME"});
		preprocessorFlags_ = shader_->getFlags();
		shader_->use();
		shader_->setBlockBinding("camera", CAMBINDING);
	}

private:

	float maxBrightness_;
	float jetAngle_;
	glm::vec2 jetSize_;
	float scale_;
	float starExposure_;

	void render() override {
		ImGui::Text("I'm the Black Hole Shader");
		ImGui::SliderFloat("Star Exposure", &starExposure_, 0.f, 1.f);
		ImGui::SliderFloat("Black Hole Scale", &scale_, 0.f, 10.f);
		ImGui::SliderFloat("Max Disc Brightness", &maxBrightness_, 1.f, 10000.f);
		ImGui::SliderFloat("Jet Angle (rad)", &jetAngle_, -1.f, 2.f);
		ImGui::SliderFloat2("Jet Size", glm::value_ptr(jetSize_), 3.f, 2000.f);
		renderPreprocessorFlags();
		ImGui::Separator();

		renderRefreshMenu();
	}

	void uploadUniforms() override {
		shader_->setUniform("star_exposure", starExposure_);
		shader_->setUniform("scale", scale_);
		shader_->setUniform("max_brightness", maxBrightness_);
		shader_->setUniform("jet_angle", jetAngle_);
		shader_->setUniform("jet_size", glm::vec4(jetSize_.x, jetSize_.y, 1.f/jetSize_.x, 1.f/ jetSize_.y));
	};


	void storeConfig(boost::json::object& obj) override {
		obj["brightness"] = maxBrightness_;
		obj["jetAngle"] = jetAngle_;
		obj["jetSize"] = {jetSize_.x, jetSize_.y};
		obj["scale"] = scale_;
		storePreprocessorFlags(obj);
	}

	void loadConfig(boost::json::object& obj) override {
		jhelper::getValue(obj, "brightness", maxBrightness_);
		jhelper::getValue(obj, "jetAngle", jetAngle_);
		jhelper::getValue(obj, "jetSize", jetSize_);
		jhelper::getValue(obj, "scale", scale_);
		loadPreprocessorFlags(obj);
		update();
	}
};

class ParticleDiscGui : public GuiElement {
public:
	ParticleDiscGui() 
		: disc_(std::make_shared<ParticleDisc>(3.f, 25.f, DISKBINDING))
	{
		density_ = disc_->getDensity();
		opacity_ = disc_->getOpacity();
		temp_ = disc_->getTemp();
		size_ = disc_->getRad();
	}
	void storeConfig(boost::json::object& obj) override {
		obj["density"] = density_;
		obj["opacity"] = opacity_;
		obj["temp"] = temp_;
		obj["size"] = { size_.x, size_.y };
		return;
	}

	void loadConfig(boost::json::object& obj) override {
		jhelper::getValue(obj, "density", density_);
		jhelper::getValue(obj, "opacity", opacity_);
		jhelper::getValue(obj, "temp", temp_);
		jhelper::getValue(obj, "size", size_);
		disc_ = std::make_shared<ParticleDisc>(size_.x, size_.y, DISKBINDING);
		update();
		return;
	}

private:
	std::shared_ptr<ParticleDisc> disc_;

	void update() override
	{
		disc_->setDensity(density_);
		disc_->setOpacity(opacity_);
		disc_->setTemp(temp_);
		disc_->setRad(size_.x, size_.y);
		disc_->uploadData();
		changed_ = false;
	}

	void render() override
	{
		ImGui::Text("Accretion Disc Settings");
		ImGui::SliderFloat("Density", &density_, 0.f, 1.f);
		ImGui::SliderFloat("Opacity", &opacity_, 0.f, 1.f);
		ImGui::SliderFloat("Temperature (K)", &temp_, 1000.f, 100000.f);
		ImGui::SliderFloat2("Size", glm::value_ptr(size_), 3.f, 2000.f);

		renderRefreshMenu();
	}

	float density_;
	float opacity_;
	float temp_;
	glm::vec2 size_;

};

class QuadShaderGui : public ShaderGui {
public:

	QuadShaderGui()
		: gamma_(2.2f)
		, exposure_(1.f)
		, bloom_(false)
		, tonemap_(false) {
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