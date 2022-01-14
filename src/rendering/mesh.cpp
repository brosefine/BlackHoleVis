#include <rendering/mesh.h>
#include <helpers/RootDir.h>


#include <tiny_obj_loader.h>

#include <iostream>
#include <map>

Mesh::~Mesh() {
	glDeleteBuffers(1, &EBO_);
	glDeleteBuffers(2, &VBO_);
	glDeleteVertexArrays(1, &VAO_);
}

Mesh::Mesh()
	: VAO_(0)
	, VBO_(0)
	, EBO_(0) {}

Mesh::Mesh(std::vector<glm::vec3> pos, std::vector<glm::vec2> uv, std::vector<unsigned int> indxs)
	: indices_(indxs)
	, has_texCoords_(true)
	, VAO_(0)
	, VBO_(0)
	, EBO_(0){
	if (pos.size() != uv.size()) {
		std::cerr << "[Error][Mesh] Number of position vectors must match number of uv coordinates." << std::endl;
		return;
	}
	for (int i = 0; i < pos.size(); ++i) {
		vertices_.push_back({ pos.at(i), uv.at(i) });
	}
	createMesh();
}

Mesh::Mesh(std::vector<Vertex> verts, std::vector<unsigned int> indxs)
	: vertices_(verts), indices_(indxs) 
	, has_texCoords_(true)
	, VAO_(0)
	, VBO_(0)
	, EBO_(0) {

	createMesh();
}

// very basic .obj loading using tinyobjloader
// supports position + uv coordinates
Mesh::Mesh(std::string filename)
	: has_texCoords_(true)
	, VAO_(0)
	, VBO_(0)
	, EBO_(0) {

	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;
	// materials won't be used
	std::vector<tinyobj::material_t> materials;
	std::string err, warn, path = ROOT_DIR "resources/meshes/" + filename;

	bool success = tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, path.c_str());

	if (!success) {
		std::cerr << "[tinyobj] " << err << std::endl;
		return;
	}

	if (err.size() > 0) {
		std::cout << "[tinyobj] " << warn << std::endl;
	}

	std::map<int, size_t> indexMap;
	for (auto const& shape : shapes) {
		size_t indexOffset = 0;
		for (size_t const& face : shape.mesh.num_face_vertices) {

			for (size_t vert = 0; vert < face; ++vert) {

				Vertex meshVert;

				tinyobj::index_t idx = shape.mesh.indices.at(indexOffset + vert);

				if (idx.vertex_index < 0) {
					std::cerr << "[mesh] not all vertices have pos coords" << std::endl;
					return;
				}

				
				if (indexMap.contains(idx.vertex_index)) {
					indices_.push_back(indexMap.at(idx.vertex_index));
					continue;
				}
				// add vertices only once

				meshVert.position.x = attributes.vertices[3 * size_t(idx.vertex_index) + 0];
				meshVert.position.y = attributes.vertices[3 * size_t(idx.vertex_index) + 1];
				meshVert.position.z = attributes.vertices[3 * size_t(idx.vertex_index) + 2];

				if (idx.texcoord_index >= 0) {
					meshVert.uvCoord.x = attributes.texcoords[2 * size_t(idx.texcoord_index) + 0];
					meshVert.uvCoord.y = attributes.texcoords[2 * size_t(idx.texcoord_index) + 1];
				}
				else {
					if(has_texCoords_)
						std::cerr << "[mesh] not all vertices have tex coords" << std::endl;
					has_texCoords_ = false;
					meshVert.uvCoord.x = 0.0f;
					meshVert.uvCoord.y = 0.0f;
				}

				vertices_.push_back(meshVert);
				indices_.push_back(vertices_.size() - 1);
				indexMap.insert({ idx.vertex_index, vertices_.size() - 1 });
			}
			indexOffset += face;
		}
	}

	createMesh();
}



void Mesh::draw(int drawMode) const {
	glBindVertexArray(VAO_);
	glDrawElements(drawMode, indices_.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::createMesh() {
	glGenVertexArrays(1, &VAO_);
	glGenBuffers(1, &VBO_);
	glGenBuffers(1, &EBO_);

	glBindVertexArray(VAO_);
	// vertices
	glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);
	// elements (=indices)
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);
	// vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uvCoord));

	glBindVertexArray(0);
}

Quad::Quad()
	:Mesh(){
	static std::vector<glm::vec3> quadPositions{
	{-1.0f, -1.0f, 0.0f},
	{-1.0f, 1.0f, 0.0f},
	{1.0f, -1.0f, 0.0f},
	{1.0f, 1.0f, 0.0f}
	};

	// texCoords
	static std::vector<glm::vec2> quadUVs{
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f}
	};

	for (int i = 0; i < quadPositions.size(); ++i) {
		vertices_.push_back({ quadPositions.at(i), quadUVs.at(i) });
	}

	indices_ = std::vector<unsigned int>{
		0, 1, 2,
		2, 1, 3
	};

	createMesh();
}