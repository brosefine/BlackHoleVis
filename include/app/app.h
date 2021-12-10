#include <gui/gui.h>
#include <rendering/window.h>
#include <helpers/Timer.hpp>

class GLApp {
public:

	GLApp(int width = 800, int height = 600, std::string const& name = "GL APP");

	void renderLoop();

protected:

	GLWindow window_;
	Gui gui_;
	FrameTimer frameTimer_;

	bool showGui_;

	virtual void renderContent() = 0;
	virtual void processKeyboardInput() {}
	virtual void renderGui() {}
};