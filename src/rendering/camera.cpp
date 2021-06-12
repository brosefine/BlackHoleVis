#include <rendering/camera.h>
#include <cmath>
#include <iostream>

Camera::Camera() 
	: position_({0, 0, 0})
	, upDir_({0, 1, 0})
	, front_({0, 0, -1})
	, yaw_(YAW)
	, pitch_(PITCH)
	, fov_(FOV)
	, translationSpeed_(1.f)
	, rotationSpeed_(.1f)
    , mouseInitialized_(false)
{
	calculateCameraVectors();
}

Camera::Camera(glm::vec3 pos, glm::vec3 up, glm::vec3 front)
	: position_(pos)
	, upDir_(up)
	, front_(front)
	, yaw_(YAW)
	, pitch_(PITCH)
	, fov_(FOV)
	, translationSpeed_(1.f)
	, rotationSpeed_(.1f)
    , mouseInitialized_(false) 
{
	calculateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
	return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::getProjectionMatrix(float aspect, float near, float far) const {
	return glm::perspective(fov_, aspect, near, far);
}

void Camera::keyBoardInput(GLFWwindow* window, float dt) {

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        translationSpeed_+= .5f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        if (translationSpeed_ >= 1)
            translationSpeed_-= .5f;

    float stepSize = translationSpeed_ * dt;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        position_ += front_ * stepSize;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        position_ -= front_ * stepSize;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        position_ -= right_ * stepSize;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        position_ += right_ * stepSize;
}

void Camera::mouseInput(GLFWwindow* window) {

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if (!mouseInitialized_) {
        lastMouseX_ = x;
        lastMouseY_ = y;
        mouseInitialized_ = true;
    }

    yaw_ += (x - lastMouseX_) * rotationSpeed_;
    pitch_ += -(y - lastMouseY_) * rotationSpeed_;
    pitch_ = std::max(std::min(pitch_, 89.f), -89.f);

    lastMouseX_ = x;
    lastMouseY_ = y;

    calculateCameraVectors();
}

void Camera::calculateCameraVectors() {

    auto yawRad = glm::radians(yaw_);
    auto pitchRad = glm::radians(pitch_);
    front_.x = std::cos(yawRad) * std::cos(pitchRad);
    front_.y = std::sin(pitchRad);
    front_.z = std::sin(yawRad) * std::cos(pitchRad);

    front_ = glm::normalize(front_);
    right_ = glm::normalize(glm::cross(front_, upDir_));
    up_ = glm::normalize(glm::cross(right_, front_));
}
