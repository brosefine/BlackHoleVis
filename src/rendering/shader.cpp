#include <rendering/shader.h>

#include <helpers/RootDir.h>
#include <glm/gtc/type_ptr.hpp>

#include <vector>


Shader::Shader(std::string vsPath, std::string fsPath) {
	// vs = vertex shader, fs = fragment shader
	std::string vsCode, fsCode;
	std::ifstream vsFile, fsFile;

	vsFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fsFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	// open files and read contents
	try {
		vsFile.open(ROOT_DIR "resources/shaders/" + vsPath);
		fsFile.open(ROOT_DIR "resources/shaders/" + fsPath);
		// won't need these stream objects after file was read
		std::stringstream vsStream, fsStream;
		// read
		vsStream << vsFile.rdbuf();
		fsStream << fsFile.rdbuf();
		// close
		vsFile.close();
		fsFile.close();
		// to string
		vsCode = vsStream.str();
		fsCode = fsStream.str();
	}
	catch (std::ifstream::failure& error) {
		std::cout << "[Error][Shader] File not read" << std::endl;
	}

	unsigned int vsID, fsID;
	const char *vsCodeChar = vsCode.c_str(), *fsCodeChar = fsCode.c_str();
	// compile vertex shader
	vsID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsID, 1, &vsCodeChar, NULL);
	glCompileShader(vsID);
	checkCompileErrors(vsID, vsPath);
	// compile fragment shader
	fsID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fsID, 1, &fsCodeChar, NULL);
	glCompileShader(fsID);
	checkCompileErrors(fsID, fsPath);
	// compile combined shader
	ID_ = glCreateProgram();
	glAttachShader(ID_, vsID);
	glAttachShader(ID_, fsID);
	glLinkProgram(ID_);
	checkCompileErrors(ID_, "program");

	glDeleteShader(vsID);
	glDeleteShader(fsID);
}

void Shader::use() {
	glUseProgram(ID_);
}

void Shader::setUniform(const std::string& name, bool value) {
	glUniform1i(glGetUniformLocation(ID_, name.c_str()), (int)value);
}

void Shader::setUniform(const std::string& name, int value) {
	glUniform1i(glGetUniformLocation(ID_, name.c_str()), value);
}

void Shader::setUniform(const std::string& name, float value) {
	glUniform1f(glGetUniformLocation(ID_, name.c_str()), value);
}

void Shader::setUniform(const std::string& name, glm::vec3 value) {
	glUniform3fv(glGetUniformLocation(ID_, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setUniform(const std::string& name, glm::mat4 value) {
	glUniformMatrix4fv(glGetUniformLocation(ID_, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setBlockBinding(const std::string& name, unsigned int binding) {
	glUniformBlockBinding(ID_, glGetUniformBlockIndex(ID_, name.c_str()), binding);
}

void Shader::checkCompileErrors(int shader, const std::string& file) {
	
	int compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		int maxLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLen);
		std::vector<char> log;
		log.reserve(maxLen);
		glGetShaderInfoLog(shader, maxLen, NULL, log.data());

		std::cout << "[Error][Shader] Compilation error at: " << file << "\n" << log.data() << std::endl;
	}
}

void Shader::checkLinkErrors(int shader) {
	int compiled;
	glGetShaderiv(shader, GL_LINK_STATUS, &compiled);

	if (!compiled) {
		int maxLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLen);
		std::vector<char> log;
		log.reserve(maxLen);
		glGetShaderInfoLog(shader, maxLen, NULL, log.data());

		std::cout << "[Error][Shader] Linking error: \n" << log.data() << std::endl;
	}
}
