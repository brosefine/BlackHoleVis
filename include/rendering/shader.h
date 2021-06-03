#pragma once

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
	// read shader file contents and compile shader code
	Shader(std::string vsPath, std::string fsPath);

	void use();
	// set uniforms
	void setUniform(const std::string& name, bool value);
	void setUniform(const std::string& name, int value);
	void setUniform(const std::string& name, float value);

	unsigned int getID() const { return ID_; }

private:
	unsigned int ID_;
	void checkCompileErrors(int shader, const std::string& file);
	void checkLinkErrors(int shader);
};