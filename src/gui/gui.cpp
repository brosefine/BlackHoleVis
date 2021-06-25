#include <gui/gui.h>

BHVGui::BHVGui(GLFWwindow *win)
	: showDemoWin_(true)
	, blackHoleElement_()
	, selectedShader_(0){
	initImGui(win);
	initElements();
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

	renderDemo();

	// Show Shader window
	renderShaderWindow();

	// Show BlackHole Window
	ImGui::Begin("Black Hole Settings");
	blackHoleElement_.show();
	ImGui::End();

	ImGui::Render();
}

void BHVGui::renderEnd() {
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void BHVGui::initImGui(GLFWwindow* win) {
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

void BHVGui::initElements() {
	shaderElements_.push_back(std::make_shared<NewtonShaderGui>());
	shaderElements_.push_back(std::make_shared<TestShaderGui>());
}

void BHVGui::renderDemo() {
	if (showDemoWin_)
		ImGui::ShowDemoWindow(&showDemoWin_);
}

void BHVGui::renderShaderWindow() {
	ImGui::Begin("Shader Settigns");

	if (ImGui::BeginListBox("Shader Selection")) {

		for (int i = 0; i < shaderElements_.size(); ++i) {
			auto shader = shaderElements_.at(i);
			if (ImGui::Selectable(shader->getName().c_str(), false))
				selectedShader_ = i;
		}
		ImGui::EndListBox();
	}

	shaderElements_.at(selectedShader_)->show();

	ImGui::End();
}
