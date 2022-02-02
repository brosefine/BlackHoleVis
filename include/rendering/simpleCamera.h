#pragma once

#include <rendering/UBOCamera.h>

#include <vector>

class SimpleCamera : public UBOCamera{
	inline static float YAW = -90.f;
	inline static float PITCH = 0.f;
	inline static float FOV = 45.f;
	inline static float NEAR = 1.f;
	inline static float FAR = 1000.f;

public:

	SimpleCamera();
	SimpleCamera(glm::vec3 pos, glm::vec3 up, glm::vec3 front, float fov = FOV);

	glm::mat4 getViewMatrix();
	glm::mat4 getProjectionMatrix(float aspect, float near = NEAR, float far = FAR);
	CameraData getData() const { return data_; }
	glm::vec3 getPosition() const { return position_; }
	glm::vec3 getFront() const { return front_; }
	glm::vec3 getUp() const { return up_; }
	glm::vec3 getRight() const { return right_; }

	bool hasChanged() const { return changed_; }
	void update(int windowWidth, int windowHeight) override;

	void setSpeed(float speed) { translationSpeed_ = speed; }
	void setFront(glm::vec3 front);
	void setPos(glm::vec3 pos);
	void setUp(glm::vec3 up);

	void keyBoardInput(GLFWwindow *window, float dt);
	void mouseInput(GLFWwindow* window);

private:

	void uploadData(int windowWidth, int windowHeight);

	// camera

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
	float translationFactor_;
	float rotationSpeed_;

	double lastMouseX_;
	double lastMouseY_;
	bool mouseInitialized_;

	bool mouseMotion_;
};