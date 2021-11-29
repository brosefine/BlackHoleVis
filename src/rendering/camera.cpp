#include <cmath>
#include <iostream>
#include <numeric>

#include <helpers/uboBindings.h>
#include <rendering/camera.h>
#include <glm/gtx/string_cast.hpp>


Camera::Camera()
    : ubo_(0)
    , position_({ 0, 0, 0 })
    , upDir_({ 0, 1, 0 })
    , yaw_(YAW)
    , pitch_(PITCH)
    , phi_(0.f)
    , theta_(0.f)
    , fov_(FOV)
    , translationSpeed_(1.f)
    , rotationSpeed_(.1f)
    , rotationSpeedX_(0.f)
    , rotationSpeedY_(0.f)
    , distanceSpeed_(0.f)
    , speedScale_(1.f)
    , mouseInitialized_(false)
    , changed_(true)
    , mouseMotion_(false)
    , lockedMode_(false)
    , friction_(true)
{
	calculateCameraVectors();
    bind();
}

Camera::Camera(glm::vec3 pos, glm::vec3 up, glm::vec3 front)
	: position_(pos)
	, upDir_(up)
    , yaw_(0.f)
    , pitch_(0.f)
    , phi_(0.f)
    , theta_(0.f)
	, fov_(FOV)
	, translationSpeed_(1.f)
	, rotationSpeed_(.1f)
    , rotationSpeedX_(0.f)
    , rotationSpeedY_(0.f)
    , distanceSpeed_(0.f)
    , speedScale_(1.f)
    , mouseInitialized_(false) 
    , changed_(true)
    , mouseMotion_(false) 
    , lockedMode_(false)
    , friction_(true)
{
    setFront(front);
    bind();
}

glm::mat4 Camera::getViewMatrix() {
	return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::getProjectionMatrix(float aspect, float near, float far) {
	return glm::perspective(glm::radians(fov_), aspect, near, far);
}

float Camera::getAvgSpeed() const {
    return std::reduce(speedHistory_.begin(), speedHistory_.end()) / (float)speedHistory_.size();
}

float Camera::getAvgSpeed(float newSpeed) {
    speedHistory_.push_back(newSpeed);
    if (speedHistory_.size() > SMOOTHSPEED) {
        speedHistory_.pop_front();
    }
    return getAvgSpeed();
}

glm::mat4 Camera::getBoost(float dt) {
    glm::vec3 vel = getCurrentVel();
    float speed = glm::length(vel);
    if (speed <= 1e-10)
        return getBoostFromVel(glm::vec3(1,0,0), 0.f);

    float avgSpeed = getAvgSpeed(speed / dt);
    return getBoostFromVel(glm::normalize(vel), avgSpeed*speedScale_);
}

glm::mat4 Camera::getBoostFromVel(glm::vec3 velocity) const {
    return getBoostFromVel(glm::normalize(velocity), glm::length(velocity));
}

glm::mat4 Camera::getBoostFromVel(glm::vec3 dir, float speed) const {
    // limit camera speed
    speed = glm::min(speed, 0.9f);
    glm::vec3 velocity = dir * speed;
    float gamma = 1.0 / glm::sqrt(1 - speed);
    float fact = (speed > 0.f) ? ((gamma - 1) / speed) : 0.f;
    glm::mat4 lorentz(
        gamma, gamma * velocity.x, gamma * velocity.y, gamma * velocity.z, // col 0
        velocity.x * gamma, 1 + fact * velocity.x * velocity.x, fact * velocity.y * velocity.x, fact * velocity.z * velocity.x,	// col 1
        velocity.y * gamma, fact * velocity.x * velocity.y, 1 + fact * velocity.y * velocity.y, fact * velocity.z * velocity.y,	// col 2
        velocity.z * gamma, fact * velocity.x * velocity.z, fact * velocity.y * velocity.z, 1 + fact * velocity.z * velocity.z	// col 3
    );
    return lorentz;
}

void Camera::update(int windowWidth, int windowHeight) {
    uploadData(windowWidth, windowHeight);
    changed_ = false;
}

void Camera::setLockedMode(bool lock){
    lockedMode_ = lock;
    speedHistory_.clear();
    if (lockedMode_) {
        yaw_ = YAW;
        pitch_ = PITCH;
        calculatePhiThetaFromPosition();
    }
    else {
        setFront(front_);
    }
}

void Camera::setFront(glm::vec3 front) {
    // calculate yaw and pitch from front vector
    front_ = glm::normalize(front);
    yaw_ = glm::degrees(std::atan2(front_.z, front_.x));
    pitch_ = glm::degrees(std::atan2(front_.y, std::sqrt(front_.x * front_.x + front_.z * front_.z)));
    pitch_ = std::clamp(pitch_, -89.f, 89.f);
    calculateCameraVectors();
    changed_ = true;
}

void Camera::setPos(glm::vec3 pos) {
    position_ = pos;
    prevPosition_ = pos;
    changed_ = true;
}

void Camera::setUp(glm::vec3 up) {
    up_ = up;
    changed_ = true;
    calculateCameraVectors();
}

void Camera::processInput(GLFWwindow* window, float dt) {
    prevPosition_ = position_;
    keyBoardInput(window, dt);
    mouseInput(window, dt);
}

void Camera::keyBoardInput(GLFWwindow* window, float dt) {

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        translationFactor_ = 10.f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
        translationFactor_ = 1.f;
    }

    float stepSize = translationSpeed_ * translationFactor_ * dt;

    if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) {
        fov_ -= stepSize;
        changed_ = true;
    }else if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) {
        fov_ += stepSize;
        changed_ = true;
    }
    fov_ = std::clamp(fov_, 10.f, 180.f);


    if (lockedMode_) {
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            rotationSpeedX_ += stepSize;
            changed_ = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            rotationSpeedX_ -= stepSize;
            changed_ = true;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            rotationSpeedY_ += stepSize;
            changed_ = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            rotationSpeedY_ -= stepSize;
            changed_ = true;
        }
    }
    else {

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
}

void Camera::mouseInput(GLFWwindow* window, float dt) {
    if (lockedMode_) mouseInputLocked(window, dt);
    else mouseInputFree(window);
}

void Camera::mouseInputLocked(GLFWwindow* window, float dt) {
    static bool rotation = false, rotationInit = false, 
        distance = false, distanceInit = false,
        lookAround = false, lookAroundInit = false;

    // get mouse button input
    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    bool ctrl = !shift && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
    bool leftMouse = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool rightMouse = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    if (!shift) {
        distance = false;
        distanceInit = false;
        rotation = false;
        rotationInit = false;
    } else {
        if (leftMouse) {
            rotation = true;
            distance = false;
            distanceInit = false;
        }
        else {
            rotation = false;
            rotationInit = false;

            if (rightMouse) {
                distance = true;
            }
            else {
                distance = false;
                distanceInit = false;
            }
        }
    } 

    if (ctrl && leftMouse) {
        lookAround = true;
    }
    else {
        lookAround = false;
        lookAroundInit = false;
    }

    // get mouse position
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (rotation) {

        if (!rotationInit) {
            lastMouseX_ = x;
            lastMouseY_ = y;
            rotationInit = true;
        }
        rotationSpeedX_ = x - lastMouseX_;
        rotationSpeedY_ = y - lastMouseY_;
    } else if (distance) {

        if (!distanceInit) {
            lastMouseY_ = y;
            distanceInit = true;
        }
        distanceSpeed_ = y - lastMouseY_;
    }
    else if (lookAround) {
        if (!lookAroundInit) {
            lastMouseX_ = x;
            lastMouseY_ = y;
            lookAroundInit = true;
        }

        // update camera orientation
        yaw_ += (x - lastMouseX_) * rotationSpeed_;
        yaw_ = glm::mod(yaw_, 360.f);
        pitch_ += -(y - lastMouseY_) * rotationSpeed_;
        pitch_ = std::clamp(pitch_, -89.f, 89.f);

        lastMouseX_ = x;
        lastMouseY_ = y;

    }

    if (!rotation && friction_) {
        rotationSpeedX_ *= 0.99f ;
        rotationSpeedX_ = abs(rotationSpeedX_) < 0.000001f ? 0.f : rotationSpeedX_;

        rotationSpeedY_ *= 0.99f;
        rotationSpeedY_ = abs(rotationSpeedY_) < 0.000001f ? 0.f : rotationSpeedY_;
    }

    if (!distance && friction_) {
        distanceSpeed_ *= 0.99f;
        distanceSpeed_ = abs(distanceSpeed_) < 0.000001f ? 0.f : distanceSpeed_;
    }

    // update camera position
    float factor = 0.1f * dt;
    position_ -= front_ * distanceSpeed_ * factor;
    phi_ += rotationSpeedX_ * factor;
    phi_ = glm::mod(phi_,360.f);
    theta_ += rotationSpeedY_ * factor;
    theta_ = glm::mod(theta_,360.f);
    // calculate new position
    float r = glm::length(position_);
    float phirad = glm::radians(phi_), thrad = glm::radians(theta_);
    position_.x = r * sin(phirad) * sin(thrad);
    position_.y = r * cos(thrad);
    position_.z = r * cos(phirad) * sin(thrad);

    front_ = -position_;
    calculateBaseVectorsFromFront();

    changed_ = true;
}

void Camera::mouseInputFree(GLFWwindow* window) {
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
    pitch_ = std::clamp(pitch_, -89.f, 89.f);

    lastMouseX_ = x;
    lastMouseY_ = y;

    changed_ = true;
    calculateCameraVectors();
}

void Camera::uploadData(int windowWidth, int windowHeight) {
    
    data_.camPos_ = getPosition();
    data_.projectionViewInverse_ = glm::inverse(getProjectionMatrix((float)windowWidth / windowHeight) * getViewMatrix());

    if (lockedMode_) {
        glm::vec3 front = calculateFront();
        glm::mat4 viewRot = glm::lookAt(glm::vec3(0.f), front, up_);
        data_.projectionInverse_ = glm::inverse(getProjectionMatrix((float)windowWidth / windowHeight) * viewRot);
    }
    else {
        data_.projectionInverse_ = glm::inverse(getProjectionMatrix((float)windowWidth / windowHeight));
    }
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraData), &data_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Camera::bind() {
    glGenBuffers(1, &ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, CAMBINDING, ubo_);
}

void Camera::calculateCameraVectors() {

    front_ = calculateFront();
    calculateBaseVectorsFromFront();
}

glm::vec3 Camera::calculateFront() const {
    // calculate front vector from pitch and yaw
    auto yawRad = glm::radians(yaw_);
    auto pitchRad = glm::radians(pitch_);
    glm::vec3 front;
    front.x = std::cos(yawRad) * std::cos(pitchRad);
    front.y = std::sin(pitchRad);
    front.z = std::sin(yawRad) * std::cos(pitchRad);

    return front;
}

void Camera::calculateBaseVectorsFromFront() {
    static float flip = 1.0;
    // update right and up to be 90 degrees to front
    front_ = glm::normalize(front_);
    
    // avoid sudden flip of view - not perfect
    glm::vec3 tmpRight= glm::normalize(glm::cross(front_, flip * upDir_));
    // check if right would flip
    if (glm::dot(tmpRight, right_) < 0) {
        flip = -flip;
        right_ = glm::normalize(glm::cross(front_, flip * upDir_));
    }     else {
        right_ = tmpRight;
    }
    
    up_ = glm::normalize(glm::cross(right_, front_));

}

void Camera::calculatePhiThetaFromPosition() {
    phi_ = glm::degrees(std::atan2(position_.x, position_.z));
    theta_ = glm::degrees(std::atan2(std::sqrt(position_.z * position_.z + position_.x * position_.x), position_.y));
    std::cout << "Phi: " << phi_ << " Theta: " << theta_ << std::endl;
}

