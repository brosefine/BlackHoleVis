#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <deque>
#include <algorithm>


const float YAW = -90.f;
const float PITCH = 0.f;
const float FOV = 45.f;
const float NEAR = 1.f;
const float FAR = 100.f;
const int SMOOTHSPEED = 10;

class Camera {
	struct CameraData {
		glm::mat4 projectionViewInverse_;
		glm::mat4 projectionInverse_;
		glm::vec3 camPos_;
	};

public:

	Camera();
	Camera(glm::vec3 pos, glm::vec3 up, glm::vec3 front);

	glm::mat4 getViewMatrix();
	glm::mat4 getProjectionMatrix(float aspect, float near = NEAR, float far = FAR);
	CameraData getData() const { return data_; }
	glm::vec3 getPosition() const { return position_; }
	glm::vec3 getFront() const { return front_; }
	glm::vec3 getUp() const { return up_; }
	glm::vec3 getRight() const { return right_; }
	glm::vec3 getCurrentVel() const { return position_ - prevPosition_; }
	float getAvgSpeed() const;
	float getAvgSpeed(float newSpeed);
	float getTheta() const { return theta_; }
	float getPhi() const { return phi_; }
	float getThetaRad() const { return glm::radians(theta_); }
	float getPhiRad() const { return glm::radians(phi_); }
	float getSpeedScale() const { return speedScale_; }
	bool isLocked() const { return lockedMode_; }
	
	glm::mat4 getBoostLocal(float dt);
	glm::mat4 getBoostGlobal(float dt);
	glm::mat4 getBoostFromVel(glm::vec3 velocity) const;
	glm::mat4 getBoostFromVel(glm::vec3 dir, float speed) const;

	bool hasChanged() const { return changed_; }
	void update(int windowWidth, int windowHeight);

	void setSpeed(float speed) { translationSpeed_ = speed; }
	void setFront(glm::vec3 front);
	void setPos(glm::vec3 pos);
	void setUp(glm::vec3 up);
	void setLockedMode(bool lock);
	void setFriction(bool f) { friction_ = f; }
	void setSpeedScale(float s) { speedScale_ = std::clamp(s, 0.f, 1.f); }

	void processInput(GLFWwindow* window, float dt);
	void keyBoardInput(GLFWwindow *window, float dt);
	void mouseInput(GLFWwindow* window, float dt);

private:

	// camera ubo

	unsigned int ubo_;
	CameraData data_;
	void bind();
	void uploadData(int windowWidth, int windowHeight);

	// camera

	void calculateCameraVectors();
	glm::vec3 calculateFront() const;
	void calculateBaseVectorsFromFront();
	void calculatePhiThetaFromPosition();

	void mouseInputLocked(GLFWwindow* window, float dt);
	void mouseInputFree(GLFWwindow* window);

	glm::vec3 position_;
	glm::vec3 prevPosition_;
	glm::vec3 upDir_;
	glm::vec3 up_;
	glm::vec3 front_;
	glm::vec3 right_;

	float yaw_;
	float pitch_;
	float phi_;
	float theta_;
	float fov_;

	float translationSpeed_;
	float translationFactor_;
	float rotationSpeed_;
	// variables for locked motion
	float rotationSpeedX_;
	float rotationSpeedY_;
	float distanceSpeed_;

	// for calculating an average speed over a number of frames
	std::deque<float> speedHistory_;
	float speedScale_;

	double lastMouseX_;
	double lastMouseY_;
	bool mouseInitialized_;

	bool changed_;
	bool mouseMotion_;
	// switch between two camera modes:
	// 0 = free mode, wasd = motion, mouse = rotate camera
	// 1 = locked mode, left mouse = rotate around object, right mouse = distance
	bool lockedMode_;
	bool friction_;
};