#pragma once

#include <memory>
#include <string>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <gui/guiElement.h>

class BHVGui {
public:
	BHVGui(GLFWwindow *win);

	void renderStart();
	void renderEnd();

	auto getCurrentShader() { return shaderElements_.at(selectedShader_)->getShader(); }
	auto getBlackHole() { return blackHoleElement_.getBlackHole(); }

	~BHVGui();

private:

	bool showDemoWin_;

	BlackHoleGui blackHoleElement_;

	int selectedShader_;
	std::vector<std::shared_ptr<ShaderGui>> shaderElements_;
	std::vector<const char*> shaderNames_;

	void initImGui(GLFWwindow* win);
	void initElements();
	void renderDemo();
	void renderShaderWindow();
};