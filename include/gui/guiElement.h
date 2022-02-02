#pragma once

#include <memory>
#include <string>
#include <map>

#include <imgui.h>
#include <boost/json.hpp>

#include <rendering/shader.h>

class GuiElement {
public:

	GuiElement(): changed_(false), alwaysUpdate_(false) {}
	GuiElement(std::string name): changed_(false), alwaysUpdate_(false), name_(name){}

	std::string getName() const { return name_; }
	// todo: remove old dump/readState functions
	virtual void dumpState(std::ofstream& outFile) {}
	virtual void readState(std::ifstream& inFile) {}

	virtual void storeConfig(boost::json::object& obj) {}
	virtual void loadConfig(boost::json::object& obj) {}
	
	void show() {
		if (changed_) update();
		render();
	}

protected:
	bool changed_;
	bool alwaysUpdate_;

	std::string name_;

	virtual void update() {};
	virtual void render() = 0;

	void renderRefreshMenu();
};

class ShaderGui : public GuiElement {
public:
	auto getShader() { return shader_; }
	void updateShader() { update(); }
protected:
	std::shared_ptr<ShaderBase> shader_;
	std::map<std::string, bool> preprocessorFlags_;

	void updatePreprocessorFlags();
	void renderPreprocessorFlags();
	void storePreprocessorFlags(boost::json::object& obj);
	void loadPreprocessorFlags(boost::json::object& obj);

	virtual void bindUBOs();
	virtual void update() override;
	virtual void uploadUniforms() {};

};
