#include <rendering/shader.h>

#include <helpers/RootDir.h>
#include <glm/gtc/type_ptr.hpp>

#include <vector>


Shader::Shader(std::string vsPath, std::string fsPath, std::vector<std::string> flags)
	: vsPaths_({ vsPath }), fsPaths_({ fsPath }) {
	for (auto const& flag : flags) {
		preprocessorFlags_.insert({ flag, false });
	}
	compile();
}

Shader::Shader(std::vector<std::string> vsPaths, std::vector<std::string> fsPaths, std::vector<std::string> flags)
: vsPaths_(vsPaths), fsPaths_(fsPaths) {
		for (auto const& flag : flags) {
			preprocessorFlags_.insert({ flag, false });
		}
		compile();
}

void Shader::use() {
	glUseProgram(ID_);
}

void Shader::reload() {
	glDeleteProgram(ID_);
	ID_ = 0;
	compile();
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

void Shader::setFlag(std::string flag, bool value) {
	preprocessorFlags_.at(flag) = value;
}

void Shader::compile() {
	// vs = vertex shader, fs = fragment shader
	std::string ppflags = createPreprocessorFlags();
	std::string vsCode = ppflags + readShaderFiles(vsPaths_);
	std::string fsCode = ppflags + readShaderFiles(fsPaths_);

	unsigned int vsID, fsID, ID;
	// add preprocessor definitions to shader code
	const char *vsCodeChar = vsCode.c_str(), *fsCodeChar = fsCode.c_str();
	// compile vertex shader
	vsID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsID, 1, &vsCodeChar, NULL);
	glCompileShader(vsID);
	bool vsCompiled = checkCompileErrors(vsID, "vertexShader");
	// compile fragment shader
	fsID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fsID, 1, &fsCodeChar, NULL);
	glCompileShader(fsID);
	bool fsCompiled = checkCompileErrors(fsID, "fragmentShader");
	// compile combined shader
	ID = glCreateProgram();
	glAttachShader(ID, vsID);
	glAttachShader(ID, fsID);
	glLinkProgram(ID);
	bool linked = checkCompileErrors(ID, "program");

	if(vsCompiled && fsCompiled && linked)
		ID_ = ID;
	glDeleteShader(vsID);
	glDeleteShader(fsID);
}

std::string Shader::readShaderFiles(std::vector<std::string> paths) const {

	std::string shaderCode;
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	for (auto const& path : paths) {

		try {

			file.open(ROOT_DIR "resources/shaders/" + path);
			// won't need these stream objects after file was read
			std::stringstream stream;
			// read
			stream << file.rdbuf();
			// close
			file.close();
			// to string
			shaderCode += stream.str();

		} catch (std::ifstream::failure& error) {
			std::cout << "[Error][Shader] File " << path << " not read" << std::endl;
			return "";
		}
	}

	return shaderCode;
}

std::string Shader::createPreprocessorFlags() const {
	std::string flags;
	for (auto const& flag : preprocessorFlags_) {
		if (flag.second) {
			flags += "#define " + flag.first + "\n";
		}
	}

	return flags;
}

bool Shader::checkCompileErrors(int shader, std::string name) {
	
	int compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		int maxLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLen);
		std::vector<char> log;
		log.reserve(maxLen);
		glGetShaderInfoLog(shader, maxLen, NULL, log.data());

		std::cout << "[Error][Shader] Compilation error at: " << name << "\n" << log.data() << std::endl;

		return false;
	}

	return true;
}

bool Shader::checkLinkErrors(int shader) {
	int compiled;
	glGetShaderiv(shader, GL_LINK_STATUS, &compiled);

	if (!compiled) {
		int maxLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLen);
		std::vector<char> log;
		log.reserve(maxLen);
		glGetShaderInfoLog(shader, maxLen, NULL, log.data());

		std::cout << "[Error][Shader] Linking error: \n" << log.data() << std::endl;

		return false;
	}

	return true;
}
