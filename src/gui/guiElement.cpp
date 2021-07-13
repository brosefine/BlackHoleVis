
#include <helpers/uboBindings.h>
#include <gui/guiElement.h>


//------------ GUI Element ------------ //


void GuiElement::renderRefreshMenu() {
	ImGui::Checkbox("Always refresh", &alwaysUpdate_);
	ImGui::SameLine();
	if (alwaysUpdate_ || ImGui::Button("Apply")) changed_ = true;
}

//------------ Shader GUI ------------ //

void ShaderGui::bindUBOs() {
	shader_->setBlockBinding("blackHole", BLHBINDING);
	shader_->setBlockBinding("camera", CAMBINDING);
}

void ShaderGui::renderPreprocessorFlags() {
	ImGui::Text("Preprocessor Flags");
	bool tmpFlag;
	for (auto& flag : preprocessorFlags_) {
		ImGui::Checkbox(flag.first.c_str(), &flag.second);
	}
}

void ShaderGui::update() {
	// update preprocessor flags
	for (auto& flag : preprocessorFlags_) {
		shader_->setFlag(flag.first, flag.second);
	}
	shader_->reload();
	shader_->use(),
		bindUBOs();
	uploadUniforms();
	changed_ = false;
}

//------------ Black Hole GUI ------------ //

BlackHoleGui::BlackHoleGui(): GuiElement("Black Hole"), blackHole_(new BlackHole({ 0.f, 0.f, 0.f }, 1.0e6, BLHBINDING)) {
	// default is a black hole at the origin
	// with a mass of 1e6 sun masses
	blackHole_->uploadData();
	mass_ = blackHole_->getMass();
}

void BlackHoleGui::update() {
	blackHole_->setMass(mass_);
	blackHole_->uploadData();
	changed_ = false;
}

void BlackHoleGui::render() {
	ImGui::Text("I'm the Black Hole");
	ImGui::InputFloat("Mass", &mass_, 10, 1000, "%e");
	renderRefreshMenu();
}

//------------ Newton Shader GUI ------------ //

NewtonShaderGui::NewtonShaderGui(): stepSize_(1.f), forceWeight_(1.f){
	name_ = "Newton";
	shader_ = std::shared_ptr<Shader>(new Shader("blackHole.vert", "newton.frag", 
		{ "EHSIZE", "RAYDIRTEST", "SKY", "FIRSTRK4", "DISK" }));
	preprocessorFlags_ = shader_->getFlags();
	shader_->use();
	bindUBOs();
	uploadUniforms();
}


void NewtonShaderGui::render() {
	ImGui::Text("Newton Shader Properties");
	ImGui::InputFloat("Step Size", &stepSize_, 0.01, 0.1);
	ImGui::InputFloat("Force Weight", &forceWeight_, .001, .01, "%.3e");

	renderPreprocessorFlags();
	ImGui::Separator();

	renderRefreshMenu();
}

void NewtonShaderGui::uploadUniforms() {
	shader_->setUniform("stepSize", stepSize_);
	shader_->setUniform("forceWeight", forceWeight_);
}

//------------ Test Shader GUI ------------ //

TestShaderGui::TestShaderGui() {
	name_ = "Test";
	shader_ = std::shared_ptr<Shader>(new Shader("blackHole.vert", "blackHoleTest.frag", { "SKY", "DISK" }));
	preprocessorFlags_ = shader_->getFlags();
	shader_->use();
	shader_->setBlockBinding("camera", CAMBINDING);
}

void TestShaderGui::render() {
	ImGui::Text("I'm the Test Shader");
	renderPreprocessorFlags();
	ImGui::Separator();

	renderRefreshMenu();
}

//------------ Starless Shader GUI ------------ //

StarlessShaderGui::StarlessShaderGui(): stepSize_(1.f), forceWeight_(1.f) {
	name_ = "Starless";
	shader_ = std::shared_ptr<Shader>(new Shader("blackHole.vert", "starless.frag",
		{ "EHSIZE", "RAYDIRTEST", "SKY", "FIRSTRK4", "DISK" }));
	preprocessorFlags_ = shader_->getFlags();
	shader_->use();
	bindUBOs();
	uploadUniforms();
}


void StarlessShaderGui::render() {
	ImGui::Text("Starless Shader Properties");
	ImGui::InputFloat("Step Size", &stepSize_, 1.0e-2, 0.1);
	ImGui::InputFloat("Force Weight", &forceWeight_, .01f, .1f);
	renderPreprocessorFlags();
	ImGui::Separator();

	renderRefreshMenu();
}

void StarlessShaderGui::uploadUniforms() {
	shader_->setUniform("stepSize", stepSize_);
	shader_->setUniform("potentialCoefficient", forceWeight_);
}
