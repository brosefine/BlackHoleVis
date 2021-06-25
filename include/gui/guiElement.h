#pragma once

#include <memory>
#include <string>

#include <imgui.h>

#include <rendering/shader.h>
#include <objects/blackHole.h>

class GuiElement {
public:

	GuiElement(): changed_(false){}
	GuiElement(std::string name): changed_(false), name_(name){}

	std::string getName() const { return name_; }
	
	void show() {
		if (changed_) update();
		render();
	}

protected:
	bool changed_;
	std::string name_;

	virtual void update() {};
	virtual void render() = 0;
};

class BlackHoleGui : public GuiElement {
public:
	BlackHoleGui();

	auto getBlackHole() { return blackHole_; }

private:

	std::shared_ptr<BlackHole> blackHole_;

	void update() override;
	void render() override;
};

class ShaderGui : public GuiElement {
public:
	auto getShader() { return shader_; }
protected:
	std::shared_ptr<Shader> shader_;
};

class NewtonShaderGui : public ShaderGui {
public:

	NewtonShaderGui();

private:
	void bindUBOs();
	void update() override;
	void render() override;
};

class TestShaderGui : public ShaderGui {
public:

	TestShaderGui();

private:

	void render() override;
};