#define STB_IMAGE_IMPLEMENTATION

#include <rendering/texture.h>
#include <helpers/RootDir.h>
#include <stb_image.h>

#include <iostream>
#include <glad/glad.h>

Texture::Texture(std::string filename): ID_(0) {
    glGenTextures(1, &ID_);
    glBindTexture(GL_TEXTURE_2D, ID_);

    int width, height, nrComponents;

    std::string path;
    path = ROOT_DIR "resources/textures/" + filename;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data) {

        GLenum format = GL_RED;
        if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height,
            0, format, GL_UNSIGNED_BYTE, data);
    } else {
        std::cerr << "[loadTexture] Texture failed to load: " << path << std::endl;
    }
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind() const {
    glBindTexture(GL_TEXTURE_2D, ID_);
}

CubeMap::CubeMap(std::vector<std::string> faces): ID_(0) {
    loadImages(faces);
}

void CubeMap::bind() const {
    glBindTexture(GL_TEXTURE_CUBE_MAP, ID_);
}

void CubeMap::loadImages(std::vector<std::string> faces) {
    if (faces.size() != 6) {
        std::cerr << "[laodCubeMap] Invalid number of cube map textures!" << std::endl;
    }

    glGenTextures(1, &ID_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ID_);

    int width, height, nrComponents;

    std::string path;
    for (int i = 0; i < faces.size(); ++i) {
        path = ROOT_DIR "resources/textures/" + faces.at(i);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
        if (data) {

            GLenum format = GL_RED;
            if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height,
                0, format, GL_UNSIGNED_BYTE, data);
        } else {
            std::cerr << "[laodCubeMap] Cubemap failed to load: " << path << std::endl;
        }
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

