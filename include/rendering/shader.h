#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
	// read shader file contents and compile shader code
	Shader() {};
	Shader(std::string vsPath, std::string fsPath, std::vector<std::string> flags = {});

	void use();
	void reload();
	// set uniforms
	void setUniform(const std::string& name, bool value);
	void setUniform(const std::string& name, int value);
	void setUniform(const std::string& name, float value);
	void setUniform(const std::string& name, glm::vec3 value);
	void setUniform(const std::string& name, glm::mat4 value);

	void setBlockBinding(const std::string& name, unsigned int binding);

	void setFlag(std::string flag, bool value);
	std::map<std::string, bool> getFlags() const {
		return preprocessorFlags_;
	}

	unsigned int getID() const { return ID_; }

private:
	unsigned int ID_;

	std::string vsPath_;
	std::string fsPath_;

	std::map<std::string, bool> preprocessorFlags_;
	std::string createPreprocessorFlags() const;

	void compile();
	bool checkCompileErrors(int shader, const std::string& file);
	bool checkLinkErrors(int shader);
};