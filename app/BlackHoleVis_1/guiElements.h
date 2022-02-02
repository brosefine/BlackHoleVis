#pragma once

#include <gui/guiElement.h>
#include <objects/blackHole.h>

class BlackHoleGui : public GuiElement {
public:
	BlackHoleGui();

	auto getBlackHole() { return blackHole_; }

private:

	std::shared_ptr<BlackHole> blackHole_;

	void update() override;
	void render() override;

	float mass_;
};

class NewtonShaderGui : public ShaderGui {
public:

	NewtonShaderGui();
	void dumpState(std::ofstream& outFile) override;
	void readState(std::ifstream& inFile) override;

private:
	void render() override;
	void uploadUniforms() override;

	float stepSize_;
	float forceWeight_;
};

class StarlessComputeShaderGui : public ShaderGui {
public:

	StarlessComputeShaderGui();
	void dumpState(std::ofstream& outFile) override;
	void readState(std::ifstream& inFile) override;

private:
	void render() override;
	void uploadUniforms() override;

	float stepSize_;
	float mass_;
};


class StarlessShaderGui : public ShaderGui {
public:

	StarlessShaderGui();

	void increaseWeight(float inc) { mass_ = std::min(1.5f, mass_ + inc); changed_ = true; }
	void decreaseWeight(float dec) { mass_ = std::max(0.f, mass_ - dec); changed_ = true; }

private:
	void render() override;
	void uploadUniforms() override;
	void dumpState(std::ofstream& outFile) override;
	void readState(std::ifstream& inFile) override;

	float stepSize_;
	float mass_;
	float forceWeight_;
	glm::vec4 sphere_;

};


class TestShaderGui : public ShaderGui {
public:

	TestShaderGui();

private:
	void render() override;

};