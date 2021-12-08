#define STB_IMAGE_IMPLEMENTATION

#include <rendering/texture.h>
#include <helpers/RootDir.h>
#include <stb_image.h>

#include <iostream>
#include <glad/glad.h>

Texture::Texture(std::string filename, bool srgb)
    : texId_(0)
    , width_(0)
    , height_(0)
    , target_(GL_TEXTURE_2D)
{
    glGenTextures(1, &texId_);
    bind();

    TextureParams params;
    params.srgb = srgb;

    std::string path;
    path = TEX_DIR"" + filename;
    params.data = stbi_load(path.c_str(), &params.width, &params.height, &params.nrComponents, 0);
    width_ = params.width;
    height_ = params.height;

    if (params.data) {
        setTextureFormat(params);
        createTexture(params);
    }
    else {
        std::cerr << "[loadTexture] Texture failed to load: " << path << std::endl;
    }
    stbi_image_free(params.data);

    std::vector<std::pair<GLenum, GLint>> texParameters{
        {GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
        {GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
        {GL_TEXTURE_MIN_FILTER, GL_LINEAR},
        {GL_TEXTURE_MAG_FILTER, GL_LINEAR}
    };

    setParam(texParameters);

    unbind();
}

// Texture Constructor
// user is responsible for providing a pointer to the texture data
// and all parameters of glTexImage2D via TextureParams
// image data is not freed in constructor
Texture::Texture(TextureParams const& params)
    : texId_(0)
    , width_(params.width)
    , height_(params.height)
    , target_(GL_TEXTURE_2D)
{
    glGenTextures(1, &texId_);
    bind();

    if (params.data) {
        createTexture(params);
    }
    else {
        std::cerr << "[loadTexture] Texture failed to load: manual load" << std::endl;
    }

    unbind();
}

Texture::~Texture() {
    glDeleteTextures(1, &texId_);
}

void Texture::createTexture(TextureParams const& params)
{
    glTexImage2D(params.target, params.level, params.internalFormat, params.width, params.height,
        params.border, params.format, params.type, params.data);
}

void Texture::setTextureFormat(TextureParams& params) {
    if (params.nrComponents == 3) {
        params.format = GL_RGB;
        params.internalFormat = params.srgb ? GL_SRGB : GL_RGB;
    }
    else if (params.nrComponents == 4) {
        params.format = GL_RGBA;
        params.internalFormat = params.srgb ? GL_SRGB_ALPHA : GL_RGBA;
    }

}

void Texture::setParam(GLenum param, GLfloat value) {
    setParam(std::vector<std::pair<GLenum, GLfloat>>{ { param, value } });
}

void Texture::setParam(GLenum param, GLint value) {
    setParam(std::vector<std::pair<GLenum, GLint>>{ { param, value } });
}

void Texture::setParam(std::vector<std::pair<GLenum, GLint>> params) {
    bind();
    for (auto const& [pname, param] : params) {

        glTexParameteri(target_, pname, param);
    }
    unbind();
}

void Texture::setParam(std::vector<std::pair<GLenum, GLfloat>> params) {
    bind();
    for (auto const& [pname, param] : params) {

        glTexParameterf(target_, pname, param);
    }
    unbind();
}

void Texture::generateMipMap() {
    bind();
    glGenerateMipmap(target_);
    unbind();
}

FBOTexture::FBOTexture(int width, int height)
    : Texture(GL_TEXTURE_2D)
    , fboId_(0) {
    width_ = width;
    height_ = height;

    // framebuffer
    glGenFramebuffers(1, &fboId_);
    bindFBO();
    // texture
    glGenTextures(1, &texId_);
    bind();
    TextureParams params;
    params.width = width;
    params.height = height;
    params.internalFormat = GL_RGBA32F;
    params.format = GL_RGBA;
    params.type = GL_FLOAT;
    params.data = NULL;

    createTexture(params);

    std::vector<std::pair<GLenum, GLint>> texParameters{
        {GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
        {GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
        {GL_TEXTURE_MIN_FILTER, GL_LINEAR},
        {GL_TEXTURE_MAG_FILTER, GL_LINEAR}
    };
    setParam(texParameters);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        texId_, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fboId_ = -1; texId_ = -1;
        std::cerr << "[Texture]: Framebuffer object not initialized" << std::endl;
    }

    unbind();
    unbindFBO();
}

FBOTexture::~FBOTexture() {
    for (auto const& [id, params] : rbos_) {
        glDeleteRenderbuffers(1, &id);
    }
    glDeleteTextures(1, &texId_);
    glDeleteFramebuffers(1, &fboId_);
}

void FBOTexture::resize(int width, int height) {
    width_ = width;
    height_ = height;

    bindFBO();
    bind();

    glTexImage2D(target_, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
        GL_FLOAT, NULL);
    generateMipMap();
    for (auto const& [id, params] : rbos_) {

        glBindRenderbuffer(GL_RENDERBUFFER, id);
        glRenderbufferStorage(GL_RENDERBUFFER, params.internalFormat, width_, height_);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[Texture]: Framebuffer object not resized" << std::endl;
    }
    unbind();
    unbindFBO();
}

void FBOTexture::bindImageTex(int binding, unsigned int mode) const {
    glBindImageTexture(binding, texId_, 0, GL_FALSE, 0, mode, GL_RGBA32F);
}

void FBOTexture::bindFBO() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fboId_);
}

void FBOTexture::unbindFBO() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBOTexture::createRBO(TextureParams const& params) {
    unsigned int id;
    bindFBO();
    glGenRenderbuffers(1, &id);
    glBindRenderbuffer(GL_RENDERBUFFER, id);

    glRenderbufferStorage(GL_RENDERBUFFER, params.internalFormat, width_, height_);
    glFramebufferRenderbuffer(params.target, params.attachement, GL_RENDERBUFFER, id);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteRenderbuffers(1, &id);
        std::cerr << "[Texture]: Renderbuffer object not initialized" << std::endl;
    }
    else {
        rbos_.push_back({ id, params });
    }

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    unbindFBO();


}

// set the MipMap level to write to
// user is responsible to call generateMipMaps before this function
// and to make sure that we can't read from the level we are writing to
void FBOTexture::setMipMapRenderLevel(int level){
    if (level < 0) {
        std::cerr << "[FBO] invalid mipmap level" << std::endl;
        return;
    }
    bindFBO();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        texId_, level);
    unbindFBO();
}

CubeMap::CubeMap(std::vector<std::string> faces) : Texture(GL_TEXTURE_CUBE_MAP) {
    loadImages(faces);
}

void CubeMap::loadImages(std::vector<std::string> faces) {
    if (faces.size() != 6) {
        std::cerr << "[laodCubeMap] Invalid number of cube map textures!" << std::endl;
    }

    glGenTextures(1, &texId_);
    bind();

    TextureParams params;
    std::string path;
    for (int i = 0; i < faces.size(); ++i) {
        path = TEX_DIR"" + faces.at(i);
        params.target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        params.data = stbi_load(path.c_str(), &params.width, &params.height, &params.nrComponents, 0);
        if (params.data) {
            setTextureFormat(params);
            createTexture(params);
        }
        else {
            std::cerr << "[laodCubeMap] Cubemap failed to load: " << path << std::endl;
        }
        stbi_image_free(params.data);
    }

    std::vector<std::pair<GLenum, GLint>> texParameters{
        {GL_TEXTURE_MIN_FILTER, GL_LINEAR},
        {GL_TEXTURE_MAG_FILTER, GL_LINEAR},
        {GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
        {GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
        {GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE}
    };

    setParam(texParameters);

    unbind();
}

