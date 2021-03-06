#pragma once

#include <memory>
#include <string>
#include <map>

#include <imgui.h>

#include <rendering/shader.h>
#include <objects/blackHole.h>

class GuiElement {
public:

	GuiElement(): changed_(false), alwaysUpdate_(false) {}
	GuiElement(std::string name): changed_(false), alwaysUpdate_(false), name_(name){}

	std::string getName() const { return name_; }
	virtual void dumpState(std::ofstream& outFile) {};
	virtual void readState(std::ifstream& inFile) {};
	
	void show() {
		if (changed_) update();
		render();
	}

protected:
	bool changed_;
	bool alwaysUpdate_;

	std::string name_;

	virtual void update() {};
	virtual void render() = 0;

	void renderRefreshMenu();
};

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

class ShaderGui : public GuiElement {
public:
	auto getShader() { return shader_; }
	void updateShader() { update(); }
protected:
	std::shared_ptr<ShaderBase> shader_;
	std::map<std::string, bool> preprocessorFlags_;

	void updatePreprocessorFlags();
	void renderPreprocessorFlags();

	virtual void bindUBOs();
	virtual void update() override;
	virtual void uploadUniforms() {};

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

	glm::vec3 baseColor_;
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

};

class TestShaderGui : public ShaderGui {
public:

	TestShaderGui();

private:
	void render() override;

};