#include <cmath>
#include <iostream>

#include <helpers/uboBindings.h>
#include <rendering/simpleCamera.h>

SimpleCamera::SimpleCamera() 
    : position_({0, 0, 0})
	, upDir_({0, 1, 0})
	, yaw_(YAW)
	, pitch_(PITCH)
	, fov_(glm::radians(FOV))
	, translationSpeed_(1.f)
	, rotationSpeed_(.1f)
    , mouseInitialized_(false)
    , mouseMotion_(false)
{
	calculateCameraVectors();
    init();
}

// fov in degrees
SimpleCamera::SimpleCamera(glm::vec3 pos, glm::vec3 up, glm::vec3 front, float fov)
	: position_(pos)
	, upDir_(up)
	, fov_(glm::radians(fov))
	, translationSpeed_(1.f)
	, rotationSpeed_(.1f)
    , mouseInitialized_(false) 
    , mouseMotion_(false) 
{
    setFront(front);
    init();
}

glm::mat4 SimpleCamera::getViewMatrix() {
	return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 SimpleCamera::getProjectionMatrix(float aspect, float near, float far) {
	return glm::perspective(fov_, aspect, near, far);
}

void SimpleCamera::update(int windowWidth, int windowHeight) {
    uploadData(windowWidth, windowHeight);
    changed_ = false;
}

void SimpleCamera::setFront(glm::vec3 front) {
    // calculate yaw and pitch from front vector
    front_ = glm::normalize(front);
    yaw_ = glm::degrees(std::atan2(front_.z, front_.x));
    pitch_ = glm::degrees(std::atan2(front_.y, std::sqrt(front_.x * front_.x + front_.z * front_.z)));
    pitch_ = std::max(std::min(pitch_, 89.999999f), -89.999999f);
    calculateCameraVectors();
    changed_ = true;
}

void SimpleCamera::setPos(glm::vec3 pos) {
    position_ = pos;
    changed_ = true;
}

void SimpleCamera::setUp(glm::vec3 up) {
    up_ = up;
    changed_ = true;
    calculateCameraVectors();
}

void SimpleCamera::keyBoardInput(GLFWwindow* window, float dt) {

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        translationFactor_ = 2.f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
        translationFactor_ = 1.f;
    }

    float stepSize = translationSpeed_ * translationFactor_ * dt;

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

void SimpleCamera::mouseInput(GLFWwindow* window){
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

void SimpleCamera::uploadData(int windowWidth, int windowHeight) {
    
    data_.camPos_ = getPosition();
    data_.projectionView_ = getProjectionMatrix((float)windowWidth / windowHeight) * getViewMatrix();
    data_.projectionViewInverse_ = glm::inverse(data_.projectionView_);
    

    uploadUboData();
}

void SimpleCamera::calculateCameraVectors() {

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