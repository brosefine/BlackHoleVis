#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

const float YAW = -90.f;
const float PITCH = 0.f;
const float FOV = 45.f;
const float NEAR = 1.f;
const float FAR = 100.f;

class Camera {
public:

	Camera();
	Camera(glm::vec3 pos, glm::vec3 up, glm::vec3 front);

	glm::mat4 getViewMatrix();
	glm::mat4 getProjectionMatrix(float aspect, float near = NEAR, float far = FAR);

	bool hasChanged() const { return changed_; }
	glm::vec3 getPosition() const { return position_; }

	void keyBoardInput(GLFWwindow *window, float dt);
	void mouseInput(GLFWwindow* window);

private:

	void calculateCameraVectors();

	glm::vec3 position_;
	glm::vec3 upDir_;
	glm::vec3 up_;
	glm::vec3 front_;
	glm::vec3 right_;

	float yaw_;
	float pitch_;
	float fov_;

	float translationSpeed_;
	float rotationSpeed_;

	double lastMouseX_;
	double lastMouseY_;
	bool mouseInitialized_;

	bool changed_;
	bool mouseMotion_;
};