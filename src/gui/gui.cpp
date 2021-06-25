#include <gui/gui.h>

BHVGui::BHVGui(GLFWwindow *win): showDemoWin_(true) {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(win, true);
	ImGui_ImplOpenGL3_Init();
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
}

BHVGui::~BHVGui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void BHVGui::renderStart() {

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Show GUI windows
	renderDemo();

	ImGui::Render();
}

void BHVGui::renderEnd() {
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void BHVGui::renderDemo() {
	if (showDemoWin_)
		ImGui::ShowDemoWindow(&showDemoWin_);
}
