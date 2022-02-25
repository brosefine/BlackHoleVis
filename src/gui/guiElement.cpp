
#include <helpers/uboBindings.h>
#include <gui/guiElement.h>
#include <glm/gtc/type_ptr.hpp>

#pragma region gui element
//------------ GUI Element ------------ //


void GuiElement::renderRefreshMenu() {
	ImGui::Checkbox(appendID("Always refresh").c_str(), &alwaysUpdate_);
	ImGui::SameLine();
	if (alwaysUpdate_ || ImGui::Button(appendID("Apply").c_str())) changed_ = true;
}


std::string GuiElement::appendID(std::string label) {
	return (label + "##" + name_);
}

#pragma endregion

#pragma region shader gui
//------------ Shader GUI ------------ //

void ShaderGui::bindUBOs() {
	shader_->setBlockBinding("blackHole", BLHBINDING);
	shader_->setBlockBinding("camera", CAMBINDING);
	shader_->setBlockBinding("accDisk", DISKBINDING);
}

void ShaderGui::updatePreprocessorFlags() {
	// update preprocessor flags
	for (auto& flag : preprocessorFlags_) {
		shader_->setFlag(flag.first, flag.second);
	}
}

void ShaderGui::renderPreprocessorFlags() {
	ImGui::Text("Preprocessor Flags");
	for (auto& flag : preprocessorFlags_) {
		ImGui::Checkbox(flag.first.c_str(), &flag.second);
	}
}

void ShaderGui::storePreprocessorFlags(boost::json::object& obj) {
	boost::json::object flags;
	for (auto const& [key, value] : preprocessorFlags_) {
		flags[key] = value;
	}
	obj["flags"] = flags;
}

void ShaderGui::loadPreprocessorFlags(boost::json::object& obj) {
	if (!obj.contains("flags") || obj.at("flags").kind() != boost::json::kind::object)
		return;
	auto flags = obj.at("flags").get_object();
	for (auto const& [key, value] : flags) {
		preprocessorFlags_[key.to_string()] = value.get_bool();
	}
}

void ShaderGui::update() {

	updatePreprocessorFlags();
	shader_->reload();
	shader_->use();
	bindUBOs();
	uploadUniforms();
	changed_ = false;
}



#pragma endregion
