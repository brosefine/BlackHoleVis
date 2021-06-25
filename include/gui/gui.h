#pragma once

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

class BHVGui {
public:
	BHVGui(GLFWwindow *win);

	void renderStart();
	void renderEnd();


	~BHVGui();

private:

	void renderDemo();

	bool showDemoWin_;
};