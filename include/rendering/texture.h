#pragma once

#include <vector>
#include <string>

class Texture {
public:
	Texture(std::string filename);

	void bind() const;
	unsigned int getId() const { return ID_; }

private:
	unsigned int ID_;
};

class CubeMap {
public:

	/*
	  Create a cubemap texture from 6 texture paths
	  Face order is: right, left, top, bottom, front, back
	  = +x, -x, +y, -y, +z, -z
	*/
	CubeMap(std::vector<std::string> faces);

	void bind() const;
	unsigned int getId() const { return ID_; }
private:
	unsigned int ID_;

	void loadImages(std::vector<std::string> faces);
};