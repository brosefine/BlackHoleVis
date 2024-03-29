#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Vertex {
	glm::vec3 position;
	glm::vec2 uvCoord;
};

class Mesh {
public:
	std::vector<Vertex> vertices_;
	std::vector<unsigned int>	indices_;
	bool has_texCoords_;

	Mesh(std::vector<glm::vec3> pos, std::vector<glm::vec2> uv, std::vector<unsigned int> indxs);
	Mesh(std::vector<Vertex> verts, std::vector<unsigned int> indxs);
	Mesh(std::string filename);
	Mesh();
	~Mesh();
	void draw(int drawMode) const;
protected:
	// vertex array object, vertex buffer object, element buffer object
	GLuint VAO_, VBO_, EBO_;
	void createMesh();
};

class Quad : public Mesh {
public:
	Quad();
};
