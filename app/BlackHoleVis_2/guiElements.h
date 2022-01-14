#pragma once

#include <gui/guiElement.h>
#include <objects/accretionDisk.h>
#include <helpers/uboBindings.h>
#include <helpers/json_helper.h>

#include <boost/json.hpp>
#include <glm/gtc/type_ptr.hpp>



class BlackHoleShaderGui : public ShaderGui {
public:

	BlackHoleShaderGui(): maxBrightness_(1000.f){

		name_ = "Black Hole";
		shader_ = std::make_shared<Shader>(
			std::vector<std::string>{ "ebruneton/black_hole_shader.vert" }, 
			std::vector<std::string>{"ebruneton/black_hole_shader.frag"},
			std::vector<std::string>{"DISC", "DOPPLER", "PINHOLE", "STARS"});
		preprocessorFlags_ = shader_->getFlags();
		shader_->use();
		shader_->setBlockBinding("camera", CAMBINDING);
	}

private:

	float maxBrightness_;

	void render() override {
		ImGui::Text("I'm the Black Hole Shader");
		ImGui::SliderFloat("Max Disc Brightness", &maxBrightness_, 1.f, 10000.f);
		renderPreprocessorFlags();
		ImGui::Separator();

		renderRefreshMenu();
	}

	void uploadUniforms() override {
		shader_->setUniform("max_brightness", maxBrightness_);
	};


	void storeConfig(boost::json::object& obj) override {
		obj["brightness"] = maxBrightness_;
		storePreprocessorFlags(obj);
	}

	void loadConfig(boost::json::object& obj) override {
		jhelper::getValue(obj, "brightness", maxBrightness_);
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
		ImGui::SliderFloat("Temperature (K)", &temp_, 1000.f, 10000.f);
		ImGui::SliderFloat2("Size", glm::value_ptr(size_), 3.f, 200.f);

		renderRefreshMenu();
	}

	float density_;
	float opacity_;
	float temp_;
	glm::vec2 size_;

};