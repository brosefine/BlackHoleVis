#include <bhv_app.h>

// positions
std::vector<glm::vec3> quadPositions{
	{-1.0f, -1.0f, 0.0f},
	{-1.0f, 1.0f, 0.0f},
	{1.0f, -1.0f, 0.0f},
	{1.0f, 1.0f, 0.0f}
};

// texCoords
std::vector<glm::vec2> quadUVs{
	{0.0f, 0.0f},
	{0.0f, 1.0f},
	{1.0f, 0.0f},
	{1.0f, 1.0f}
};

std::vector<unsigned int> quadIndices{
	0, 1, 2,
	2, 1, 3
};

BHVApp::BHVApp(int width, int height)
	: window_(width, height, "Black Hole Vis")
	, gui_(window_.getPtr())
	, cam_({ 0.f, 0.f, -10 }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f })
	, quad_(quadPositions, quadUVs, quadIndices)
	, sky_({ "sky_right.png", "sky_left.png", "sky_top.png", "sky_bottom.png", "sky_front.png", "sky_back.png" })
	, t0_(0.f), dt_(0.f)
{
	cam_.update(window_.getWidth(), window_.getHeight());
	sky_.bind();
}

void BHVApp::renderLoop() {

	while (!window_.shouldClose()) {

		gui_.renderStart();

		float now = glfwGetTime();
		dt_ = now - t0_;
		t0_ = now;

		cam_.keyBoardInput(window_.getPtr(), dt_);
		cam_.mouseInput(window_.getPtr());
		if (cam_.hasChanged() || window_.hasChanged())
			cam_.update(window_.getWidth(), window_.getHeight());

		glClearColor(0.5, 0.5, 0.5, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		gui_.getCurrentShader()->use();

		quad_.draw(GL_TRIANGLES);

		gui_.renderEnd();
		window_.endFrame();
	}
}
