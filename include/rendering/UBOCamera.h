#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

class UBOCamera {
protected:
	struct CameraData {
		glm::mat4 projectionView_;
		glm::mat4 projectionViewInverse_;
		glm::vec3 camPos_;
	};

public:

	UBOCamera():ubo_(0),data_(),changed_(true){}

	void use(int windowWidth, int windowHeight, bool doUpdate = true);
	virtual void update(int windowWidth, int windowHeight) = 0;

protected:
	unsigned int ubo_;
	CameraData data_;
	bool changed_;

	void init();
	void bind();
	void uploadUboData();

};