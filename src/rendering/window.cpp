#include <rendering/window.h>

#include <iostream>

BHVWindow::BHVWindow(int width, int height, std::string name)
	: width_(width), height_(height), name_(name){
	init();
}

BHVWindow::~BHVWindow() {
	glfwDestroyWindow(windowPtr_);
	glfwTerminate();
}

void BHVWindow::setHeight(int h) {
	height_ = h;
	glfwSetWindowSize(windowPtr_, width_, h);
	changed_ = true;
}

void BHVWindow::setWidth(int w) {
	width_ = w;
	glfwSetWindowSize(windowPtr_, w, height_);
	changed_ = true;
}

bool BHVWindow::shouldClose() {
	return glfwWindowShouldClose(windowPtr_);
}

void BHVWindow::endFrame() {
	glfwPollEvents();
	glfwSwapBuffers(windowPtr_);
}

void BHVWindow::init() {
	// GLFW
	glfwInit();
	// checks if correct OpenGL versions are present
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// core profile includes smaller set of functions, e.g. no backwards-compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	windowPtr_ = glfwCreateWindow(width_, height_, name_.c_str(), NULL, NULL);
	if (!windowPtr_) {
		std::cout << "Window creation failed" << std::endl;
		glfwTerminate();
		windowPtr_ = nullptr;
		return;
	}
	glfwMakeContextCurrent(windowPtr_);
	glfwSwapInterval(1); // Enable vsync
	// set this to user pointer for reference in static callback methods
	glfwSetWindowUserPointer(windowPtr_, this);
	glfwSetFramebufferSizeCallback(windowPtr_, framebufferResizeCallback);

	// GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		windowPtr_ = nullptr;
		return;
	}
}

void BHVWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	// get window user pointer to change width and height of window object
	auto windowObject = (BHVWindow*)glfwGetWindowUserPointer(window);
	windowObject->setHeight(height);
	windowObject->setWidth(width);
}

void BHVWindow::processKeyboardInputs() {

	// close window with escape
	if (glfwGetKey(windowPtr_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(windowPtr_, true);
}