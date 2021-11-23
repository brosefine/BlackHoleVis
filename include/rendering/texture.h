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
	GLint border = 0;
	GLenum format = GL_RED;
	GLenum type = GL_UNSIGNED_BYTE;
	void* data = nullptr;
};


class Texture {
public:

	// create texture from image file
	Texture():texId_(0),width_(0),height_(0){}
	Texture(std::string filename, bool srgb = false);
	Texture(TextureParams const& params);
	~Texture();

	virtual void setParam(GLenum param, GLint value);
	virtual void setParam(std::vector<std::pair<GLenum, GLint>> params);

	virtual void bind() const;
	unsigned int getTexId() const { return texId_; }

	int getWidth() const { return width_; }
	int getHeight() const { return height_; }

protected:
	unsigned int texId_;
	int width_, height_;

	void createTexture(TextureParams const& params);
	void setTextureFormat(TextureParams& params);

};

class FBOTexture: public Texture{
public:
	// create empty fbo texture
	FBOTexture(int width, int height);
	~FBOTexture();

	void resize(int width, int height);

	void bindImageTex(int binding, unsigned int mode) const;
	unsigned int getTexId() const { return texId_; }
	unsigned int getFboId() const { return fboId_; }

private:
	unsigned int fboId_;
};

class CubeMap : public Texture{
public:

	/*
	  Create a cubemap texture from 6 texture paths
	  Face order is: right, left, top, bottom, front, back
	  = +x, -x, +y, -y, +z, -z
	*/
	CubeMap(std::vector<std::string> faces);

	void bind() const override;
	void setParam(std::vector<std::pair<GLenum, GLint>> params) override;

private:

	void loadImages(std::vector<std::string> faces);
};