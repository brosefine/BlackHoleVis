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
	Shader(std::vector<std::string> vsPaths, std::vector<std::string> fsPaths, std::vector<std::string> flags = {});

	void use();
	void reload();
	// set uniforms
	void setUniform(const std::string& name, bool value);
	void setUniform(const std::string& name, int value);
	void setUniform(const std::string& name, float value);
	void setUniform(const std::string& name, glm::vec2 value);
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

	std::vector<std::string> vsPaths_;
	std::vector<std::string> fsPaths_;
	std::string versionDirective_;

	std::map<std::string, bool> preprocessorFlags_;
	std::string createPreprocessorFlags() const;

	void compile();
	std::string readShaderFiles(std::vector<std::string> paths) const;
	bool checkCompileErrors(int shader, std::string name);
	bool checkLinkErrors(int shader);
};