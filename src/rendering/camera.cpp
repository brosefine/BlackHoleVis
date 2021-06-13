#include <rendering/camera.h>
#include <cmath>
#include <iostream>

Camera::Camera() 
	: position_({0, 0, 0})
	, upDir_({0, 1, 0})
	, yaw_(YAW)
	, pitch_(PITCH)
	, fov_(FOV)
	, translationSpeed_(1.f)
	, rotationSpeed_(.1f)
    , mouseInitialized_(false)
    , changed_(true)
    , mouseMotion_(false)
{
	calculateCameraVectors();
}

Camera::Camera(glm::vec3 pos, glm::vec3 up, glm::vec3 front)
	: position_(pos)
	, upDir_(up)
	, fov_(FOV)
	, translationSpeed_(1.f)
	, rotationSpeed_(.1f)
    , mouseInitialized_(false) 
    , changed_(true)
    , mouseMotion_(false) 
{
	// calculate yaw and pitch from front vector
    yaw_ = glm::degrees(std::atan2(front.z, front.x));
    pitch_ = glm::degrees(std::asin(-front.y));
    std::cout << "front.x: " << front.x << " front.y: " << front.y << " front.z: " << front.z << std::endl;
    std::cout << "pitch: " << pitch_ << " yaw: " << yaw_ << std::endl;
    calculateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() {
	return glm::lookAt(position_, position_ + front_, up_);
    changed_ = false;
}

glm::mat4 Camera::getProjectionMatrix(float aspect, float near, float far) {
	return glm::perspective(fov_, aspect, near, far);
    changed_ = false;
}

void Camera::keyBoardInput(GLFWwindow* window, float dt) {

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        translationSpeed_ = 2.f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
        translationSpeed_ = 1.f;
    }

    float stepSize = translationSpeed_ * dt;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position_ += front_ * stepSize;
        changed_ = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position_ -= front_ * stepSize;
        changed_ = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position_ -= right_ * stepSize;
        changed_ = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position_ += right_ * stepSize;
        changed_ = true;
    }
}

void Camera::mouseInput(GLFWwindow* window){
    // get mouse button input
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        mouseMotion_ = true;
    } else {
        mouseMotion_ = false;
        mouseInitialized_ = false;
    }
        
    if (!mouseMotion_) return;
    
    // get mouse position
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if (!mouseInitialized_) {
        lastMouseX_ = x;
        lastMouseY_ = y;
        mouseInitialized_ = true;
    }
    // stop if mouse position barely changed
    if (abs(lastMouseX_ - x) < 0.000001f && abs(lastMouseY_ - y) < 0.000001f)
        return;
    // update camera orientation
    yaw_ += (x - lastMouseX_) * rotationSpeed_;
    pitch_ += -(y - lastMouseY_) * rotationSpeed_;
    pitch_ = std::max(std::min(pitch_, 89.f), -89.f);

    lastMouseX_ = x;
    lastMouseY_ = y;

    changed_ = true;
    calculateCameraVectors();
}

void Camera::calculateCameraVectors() {

    // calculate front vector from pitch and yaw
    auto yawRad = glm::radians(yaw_);
    auto pitchRad = glm::radians(pitch_);
    front_.x = std::cos(yawRad) * std::cos(pitchRad);
    front_.y = std::sin(pitchRad);
    front_.z = std::sin(yawRad) * std::cos(pitchRad);

    // update right and up to be 90 degrees to front
    front_ = glm::normalize(front_);
    right_ = glm::normalize(glm::cross(front_, upDir_));
    up_ = glm::normalize(glm::cross(right_, front_));
}
