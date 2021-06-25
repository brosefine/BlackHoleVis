#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>

class BHVWindow {
public:
	BHVWindow() {};
	BHVWindow(int width, int height, std::string name);
	~BHVWindow();

	void setWidth(int w) { width_ = w; }
	void setHeight(int h) { height_ = h; }
	int getWidth() { return width_; }
	int getHeight() { return height_; }
	GLFWwindow* getPtr() { return windowPtr_; }

	bool shouldClose();
	void endFrame();

private:

	GLFWwindow* windowPtr_;
	int width_;
	int height_;
	std::string name_;

	void init();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void processKeyboardInputs();
};