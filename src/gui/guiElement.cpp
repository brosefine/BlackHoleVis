
#include <helpers/uboBindings.h>
#include <gui/guiElement.h>


BlackHoleGui::BlackHoleGui(): GuiElement("Black Hole"), blackHole_(new BlackHole({ 0.f, 0.f, 0.f }, 1.0e6, BLHBINDING)) {
	// default is a black hole at the origin
	// with a mass of 1e6 sun masses
	blackHole_->uploadData();
}

void BlackHoleGui::update() {
	blackHole_->uploadData();
	changed_ = false;
}

void BlackHoleGui::render() {
	ImGui::Text("I'm the Black Hole");
	if (ImGui::Button("Apply")) changed_ = true;
}

NewtonShaderGui::NewtonShaderGui(): stepSize_(2.5f), forceWeight_(8.5e-4){
	name_ = "Newton";
	shader_ = std::shared_ptr<Shader>(new Shader("blackHole.vert", "newton.frag", { "EHSIZE", "TESTDIST" }));
	shader_->use();
	bindUBOs();
	uploadUniforms();
}


void NewtonShaderGui::update() {
	shader_->reload();
	shader_->use(),
	bindUBOs();
	uploadUniforms();
	changed_ = false;
}

void NewtonShaderGui::render() {
	ImGui::Text("I'm the Newton Shader");
	if (ImGui::Button("Apply")) changed_ = true;
}

void NewtonShaderGui::bindUBOs() {
	shader_->setBlockBinding("blackHole", BLHBINDING);
	shader_->setBlockBinding("camera", CAMBINDING);
}

void NewtonShaderGui::uploadUniforms() {
	shader_->setUniform("stepSize", stepSize_);
	shader_->setUniform("forceWeight", forceWeight_);
}

TestShaderGui::TestShaderGui() {
	name_ = "Test";
	shader_ = std::shared_ptr<Shader>(new Shader("blackHole.vert", "blackHoleTest.frag"));
	shader_->use();
	shader_->setBlockBinding("camera", CAMBINDING);
}

void TestShaderGui::render() {
	ImGui::Text("I'm the Test Shader");
}
