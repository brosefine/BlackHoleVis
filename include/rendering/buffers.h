#pragma once

#include <glad/glad.h>
#include <stdlib.h>

class SSBO {
public:

	SSBO(): ID_(0){}
	SSBO(size_t size, GLenum usage = GL_STATIC_COPY): ID_(0) {
		glGenBuffers(1, &ID_);
		bind();
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, usage);
		unbind();
	}

	SSBO(size_t size, void* data, GLenum usage = GL_STATIC_COPY) : ID_(0) {
		glGenBuffers(1, &ID_);
		bind();
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
		unbind();
	}

	~SSBO(){
		glDeleteBuffers(1, &ID_);
	}

	void bind() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID_);
	}

	void unbind() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void subData(size_t offset, size_t size, void* data) {
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
	}

	void bindBase(int base) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, base, ID_);
	}

private:
	GLuint ID_;
};