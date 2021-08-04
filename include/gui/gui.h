#pragma once

#include <memory>
#include <string>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

class Gui {
public:
	Gui(GLFWwindow *win);

	void newFrame();
	void render();
	void renderEnd();

	~Gui();

private:

	void initImGui(GLFWwindow* win);
};