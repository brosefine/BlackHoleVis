#pragma once

#include <vector>
#include <string>

#include <glad/glad.h>

/* Parameter container for loading texture data
* Default values are:
* srgb = false
* int nrComponents = 1;
* target = GL_TEXTURE_2D
* level = 0;
* internalFormat = GL_RED;
* width = 0;
* height = 0;
* depth = 0;
* border = 0;
* format = GL_RED;
* type = GL_UNSIGNED_BYTE;
*/
struct TextureParams {
	bool srgb = false;
	int nrComponents = 1;
	GLenum target = GL_TEXTURE_2D;
	GLint level = 0;
	GLint internalFormat = GL_RED;
	GLsizei width = 0;
	GLsizei height = 0;
	GLsizei depth = 0;
	GLint border = 0;
	GLenum format = GL_RED;
	GLenum type = GL_UNSIGNED_BYTE;
	GLenum attachement = GL_DEPTH_ATTACHMENT;
	void* data = nullptr;
};


class Texture {
public:

	// create texture from image file
	Texture(unsigned int id, GLenum target) : texId_(id) ,target_(target){}
	~Texture();

	void setParam(GLenum param, GLint value);
	void setParam(GLenum param, GLfloat value);
	void setParam(std::vector<std::pair<GLenum, GLint>> params);
	void setParam(std::vector<std::pair<GLenum, GLfloat >> params);
	void generateMipMap();

	void bind() const { glBindTexture(target_, texId_); }
	void unbind() const { glBindTexture(target_, 0); }

	unsigned int getTexId() const { return texId_; }

protected:
	unsigned int texId_;
	GLenum target_;

	virtual void createTexture(TextureParams const& params) = 0;
	void setTextureFormat(TextureParams& params);

};

class Texture2D : public Texture{
public:

	// create texture from image file
	Texture2D():Texture(0, GL_TEXTURE_2D),width_(0),height_(0){}
	Texture2D(GLenum target):Texture(0, target),width_(0),height_(0){}
	Texture2D(std::string filename, bool srgb = false);
	Texture2D(TextureParams const& params);

	int getWidth() const { return width_; }
	int getHeight() const { return height_; }

protected:
	int width_, height_;

	void createTexture(TextureParams const& params) override;

};

class FBOTexture: public Texture2D{
public:
	// create empty fbo texture
	FBOTexture(int width, int height);
	~FBOTexture();

	void createRBO(TextureParams const& params);
	void resize(int width, int height);
	void setMipMapRenderLevel(int level);

	void bindImageTex(int binding, unsigned int mode) const;
	void bindFBO() const;
	void unbindFBO() const;
	unsigned int getTexId() const { return texId_; }
	unsigned int getFboId() const { return fboId_; }

private:
	unsigned int fboId_;
	std::vector<std::pair<GLuint, TextureParams>> rbos_;
};

class CubeMap : public Texture2D {
public:

	/*
	  Create a cubemap texture from 6 texture paths
	  Face order is: right, left, top, bottom, front, back
	  = +x, -x, +y, -y, +z, -z
	*/
	CubeMap(std::vector<std::string> faces);
	CubeMap(int width, int height);

private:

	void loadImages(std::vector<std::string> faces);
};

class Texture3D : public Texture {
public:
	Texture3D(TextureParams const& params);


	int getWidth() const { return width_; }
	int getHeight() const { return height_; }
	int getDepth() const { return depth_; }

private:
	int width_, height_, depth_;
	void createTexture(TextureParams const& params) override;

};