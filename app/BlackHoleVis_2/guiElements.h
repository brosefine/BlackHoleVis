#pragma once

#include <gui/guiElement.h>
#include <objects/accretionDisk.h>
#include <helpers/uboBindings.h>


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

class ParticleDiscGui : public GuiElement {
public:
	ParticleDiscGui() 
		: disc_(std::make_shared<ParticleDisc>(DISKBINDING))
	{
		density_ = disc_->getDensity();
		opacity_ = disc_->getOpacity();
		temp_ = disc_->getTemp();
		size_ = disc_->getRad();
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
		//ImGui::SliderFloat2("Size", glm::value_ptr(size_), 3.f, 20.f);

		renderRefreshMenu();
	}

	float density_;
	float opacity_;
	float temp_;
	glm::vec2 size_;

};