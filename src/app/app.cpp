#include <app/app.h>

GLApp::GLApp(int width, int height, std::string const& name)
	: window_(width, height, name)
	, gui_(window_.getPtr())
	, showGui_(false)
	, showFps_(false)
{
}

void GLApp::renderLoop()
{
	while (!window_.shouldClose())
	{
		processKeyboardInput();
		if (showGui_) {

			gui_.newFrame();
			renderGui();
			gui_.render();
		}

		renderContent();

		if (showGui_) gui_.renderEnd();
		window_.endFrame();
		frameTimer_.measure();
	}
}

void GLApp::renderGui() {
	if (showFps_)
		renderFPSWindow();

	ImGui::Begin("Application Options");
	ImGui::Checkbox("Show FPS", &showFps_);
	ImGui::End();
}

void GLApp::renderFPSWindow() {
	ImGui::Begin("FPS");
	double avgTime = frameTimer_.getAvg();
	double avgFPS = 1.f / avgTime;
	static double printTime = 0, printFPS = 0;
	ImGui::Text(std::to_string(printTime*1e3).c_str());
	ImGui::SameLine();
	ImGui::Text("ms - ");
	ImGui::SameLine();
	ImGui::Text(std::to_string(printFPS).c_str());
	ImGui::SameLine();
	if (ImGui::Button("Update")) {
		printTime = avgTime;
		printFPS = avgFPS;
	}

	static bool showPlot = false;
	ImGui::Checkbox("Show Plot", &showPlot);
	if (showPlot) {

		static ScrollingBuffer rdata1, rdata2;
		static float t = 0;
		t += ImGui::GetIO().DeltaTime;
		rdata1.AddPoint(t, avgTime);
		rdata2.AddPoint(t, (1.0f / avgTime));

		static float history = 10.0f;
		ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");
		static ImPlotAxisFlags flags = ImPlotAxisFlags_AutoFit;

		ImGui::BulletText("Time btw. frames");

		ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
		ImPlot::SetNextPlotLimitsY(0, 0.1, ImGuiCond_Always);
		if (ImPlot::BeginPlot("##Rolling1", NULL, NULL, ImVec2(-1, 150), 0, flags, flags)) {
			ImPlot::PlotLine("dt", &rdata1.Data[0].x, &rdata1.Data[0].y, rdata1.Data.size(), 0, 2 * sizeof(float));
			ImPlot::EndPlot();
		}

		ImGui::BulletText("Frames per Second");

		ImPlot::SetNextPlotLimitsX(t - history, t, ImGuiCond_Always);
		ImPlot::SetNextPlotLimitsY(0, 200, ImGuiCond_Always);
		if (ImPlot::BeginPlot("##Rolling2", NULL, NULL, ImVec2(-1, 150), 0, flags, flags)) {
			ImPlot::PlotLine("FPS", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 2 * sizeof(float));
			ImPlot::EndPlot();
		}
	}

	ImGui::End();
}
