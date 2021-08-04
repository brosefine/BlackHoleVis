#include <bhv_app.h>
#include <fstream>

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
	, selectedShader_(0)
	, showShaders_(false)
	, showCamera_(false)
{
	cam_.update(window_.getWidth(), window_.getHeight());
	sky_.bind();
	initGuiElements();
}

void BHVApp::renderLoop() {

	while (!window_.shouldClose()) {

		gui_.newFrame();
		renderOptionsWindow();
		if (showShaders_) renderShaderWindow();
		if (showCamera_) renderCameraWindow();
		gui_.render();

		float now = glfwGetTime();
		dt_ = now - t0_;
		t0_ = now;

		cam_.keyBoardInput(window_.getPtr(), dt_);
		cam_.mouseInput(window_.getPtr());
		if (cam_.hasChanged() || window_.hasChanged())
			cam_.update(window_.getWidth(), window_.getHeight());

		glClearColor(0.5, 0.5, 0.5, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		getCurrentShader()->use();

		quad_.draw(GL_TRIANGLES);

		gui_.renderEnd();
		window_.endFrame();
	}
}

std::shared_ptr<Shader> BHVApp::getCurrentShader() {
	return shaderElements_.at(selectedShader_)->getShader();
}

void BHVApp::initGuiElements() {
	shaderElements_.push_back(std::make_shared<NewtonShaderGui>());
	shaderElements_.push_back(std::make_shared<StarlessShaderGui>());
	shaderElements_.push_back(std::make_shared<TestShaderGui>());
}

void BHVApp::renderOptionsWindow() {
	ImGui::Begin("Application Options");
	ImGui::Checkbox("Shader Settings", &showShaders_);
	ImGui::Checkbox("Camera Settings", &showCamera_);

	ImGui::Text("Save / Load current state");
	ImGui::PushItemWidth(ImGui::GetFontSize() * 7);
	if (ImGui::Button("Save")) dumpState();
	ImGui::SameLine();
	if (ImGui::Button("Load")) readState();
	ImGui::PopItemWidth();

	ImGui::End();
}

void BHVApp::renderShaderWindow() {
	ImGui::Begin("Shader Settings");
	ImGui::Text("Shader Selection");
	if (ImGui::BeginListBox("")) {

		for (int i = 0; i < shaderElements_.size(); ++i) {
			auto shader = shaderElements_.at(i);
			if (ImGui::Selectable(shader->getName().c_str(), false))
				selectedShader_ = i;
		}
		ImGui::EndListBox();
	}
	ImGui::Separator();
	shaderElements_.at(selectedShader_)->show();

	ImGui::End();
}

void BHVApp::renderCameraWindow() {

	ImGui::Begin("Camera Settings");
	ImGui::Text("Camera Position");

	glm::vec3 camPos = cam_.getPosition();
	ImGui::PushItemWidth(ImGui::GetFontSize() * 7);
	bool xChange = ImGui::InputFloat("X", &camPos.x, 0.1, 1); ImGui::SameLine();
	bool yChange = ImGui::InputFloat("Y", &camPos.y, 0.1, 1); ImGui::SameLine();
	bool zChange = ImGui::InputFloat("Z", &camPos.z, 0.1, 1);
	ImGui::PopItemWidth();

	if (ImGui::Button("Point to Black Hole"))
		cam_.setFront(-camPos);
	
	if (xChange || yChange || zChange)
		cam_.setPos(camPos);

	ImGui::End();

}

void BHVApp::dumpState() {
	std::cout << "Dump" << std::endl;

	std::ofstream outFile("dump.txt");
	outFile << "Camera\n";
	glm::vec3 camPos = cam_.getPosition();
	outFile << camPos.x << " " << camPos.y << " " << camPos.z << "\n";
	glm::vec3 camFront = cam_.getFront();
	outFile << camFront.x << " " << camFront.y << " " << camFront.z << "\n";
	glm::vec3 camUp = cam_.getUp();
	outFile << camUp.x << " " << camUp.y << " " << camUp.z << "\n";

	outFile << "Shaders\n";
	for (int i = 0; i < shaderElements_.size(); ++i) {
		outFile << "Shader " << i << "\n";
		shaderElements_.at(i)->dumpState(outFile);
		outFile << "EndShader\n";
	}

	outFile.close();
}

void BHVApp::readState() {
	std::cout << "Read" << std::endl;

	std::ifstream inFile("dump.txt");
	std::string word;
	while (inFile >> word) {
		if (word == "Camera") {
			glm::vec3 camPos = cam_.getPosition();
			inFile >> word; camPos.x = std::stof(word);
			inFile >> word; camPos.y = std::stof(word);
			inFile >> word; camPos.z = std::stof(word);
			glm::vec3 camFront = cam_.getFront();
			inFile >> word; camFront.x = std::stof(word);
			inFile >> word; camFront.y = std::stof(word);
			inFile >> word; camFront.z = std::stof(word);
			glm::vec3 camUp = cam_.getUp();
			inFile >> word; camUp.x = std::stof(word);
			inFile >> word; camUp.y = std::stof(word);
			inFile >> word; camUp.z = std::stof(word);
			
			cam_.setPos(camPos);
			cam_.setFront(camFront);
			cam_.setUp(camUp);
		}
		if (word == "Shaders") {
			inFile >> word;
			while (word == "Shader") {
				inFile >> word;
				shaderElements_.at(std::stoi(word))->readState(inFile);
				inFile >> word;
			}
		}
	}

	inFile.close();
}
