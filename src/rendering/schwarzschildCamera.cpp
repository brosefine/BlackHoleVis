#include <cmath>
#include <iostream>
#include <numeric>

#include <helpers/uboBindings.h>
#include <rendering/schwarzschildCamera.h>
#include <glm/gtx/string_cast.hpp>


SchwarzschildCamera::SchwarzschildCamera()
    : ubo_(0)
    , velocityRTP_(0,0,0)
    , positionXYZ_(1,1,1)
    , prevPositionXYZ_(1,1,1)
    , viewDirTP_(VIEWTHETA, VIEWPHI)
    , fov_(FOV)
    , translationSpeed_(1.f)
    , rotationSpeed_(.01f)
    , speedScale_(1.f)
    , mouseInitialized_(false)
    , changed_(true)
    , mouseMotion_(false)
    , lockedMode_(true)
    , friction_(true)
{
    setPosXYZ(positionXYZ_);
    calculateCameraVectors();
    bind();
}

SchwarzschildCamera::SchwarzschildCamera(glm::vec3 pos)
    : ubo_(0)
    , velocityRTP_(0, 0, 0)
    , positionXYZ_(pos)
    , prevPositionXYZ_(pos)
    , viewDirTP_(VIEWTHETA, VIEWPHI)
    , fov_(FOV)
    , translationSpeed_(1.f)
    , rotationSpeed_(.1f)
    , speedScale_(1.f)
    , mouseInitialized_(false)
    , changed_(true)
    , mouseMotion_(false)
    , lockedMode_(true)
    , friction_(true)
{
    setPosXYZ(positionXYZ_);
    calculateCameraVectors();
    bind();
}

glm::mat4 SchwarzschildCamera::getProjectionMatrix(float aspect, float near, float far) {
	return glm::perspective(fov_, aspect, near, far);
}

glm::mat3 SchwarzschildCamera::getBase3() const {
    return glm::mat3(
        right_.y, right_.z, right_.w,
        up_.y, up_.z, up_.w,
        front_.y, front_.z, front_.w
    );
}

glm::mat4 SchwarzschildCamera::getBase4() const {
    return glm::mat4(
        tau_,
        right_,
        up_,
        front_
    );
}

glm::mat3 SchwarzschildCamera::getFidoBase3() const
{
    float u = 1.f / positionRTP_.x;
    float v = glm::sqrt(1.f - u);

    glm::mat3 base = getBase3();
    base[0] = base[0] * u;// *(u / glm::sin(positionRTP_.y)); // right
    base[1] = base[1] * u;// *u;  // up
    base[2] = base[2] * v;  // front

    return base;
}

glm::mat4 SchwarzschildCamera::getFidoBase4() const
{
    float u = 1.f / positionRTP_.x;
    float v = glm::sqrt(1.f - u);

    glm::mat4 base = getBase4();
    base[0] = base[0] / v;  // tau
    base[1] = base[1] * u;// *(u / glm::sin(positionRTP_.y)); // right
    base[2] = base[2] * u;// *u;  // up
    base[3] = base[3] * v;  // front

    return base;
}

float SchwarzschildCamera::getAvgSpeed() const {
    return std::reduce(speedHistory_.begin(), speedHistory_.end()) / (float)speedHistory_.size();
}

float SchwarzschildCamera::getAvgSpeed(float newSpeed) {
    speedHistory_.push_back(newSpeed);
    if (speedHistory_.size() > SMOOTHSPEED) {
        speedHistory_.pop_front();
    }
    return getAvgSpeed();
}

glm::mat4 SchwarzschildCamera::getBoostLocal(float dt) {
    glm::mat3 globToLoc = glm::inverse(getBase3());
    glm::vec3 vel = globToLoc * getCurrentVelXYZ();
    float speed = glm::length(vel);
    if (speed <= 1e-10)
        return getBoostFromVel(glm::vec3(1,0,0), 0.f);

    float avgSpeed = getAvgSpeed(speed / dt);
    return getBoostFromVel(glm::normalize(vel), avgSpeed*speedScale_);
}

glm::mat4 SchwarzschildCamera::getBoostGlobal(float dt) {
    glm::vec3 vel = getCurrentVelXYZ();
    float speed = glm::length(vel);
    if (speed <= 1e-10)
        return getBoostFromVel(glm::vec3(1, 0, 0), 0.f);

    float avgSpeed = getAvgSpeed(speed / dt);
    return getBoostFromVel(glm::normalize(vel), avgSpeed * speedScale_);
}

glm::mat4 SchwarzschildCamera::getBoostLocalFido(float dt) {
    glm::mat3 globToLoc = glm::inverse(getFidoBase3());
    glm::vec3 vel = globToLoc * getCurrentVelXYZ();
    float speed = glm::length(vel);
    if (speed <= 1e-10)
        return getBoostFromVel(glm::vec3(1, 0, 0), 0.f);

    float avgSpeed = getAvgSpeed(speed / dt);
    return getBoostFromVel(glm::normalize(vel), avgSpeed * speedScale_);
}

glm::mat4 SchwarzschildCamera::getBoostFromVel(glm::vec3 velocity) const {
    return getBoostFromVel(glm::normalize(velocity), glm::length(velocity));
}

glm::mat4 SchwarzschildCamera::getBoostFromVel(glm::vec3 dir, float speed) const {
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

void SchwarzschildCamera::update(int windowWidth, int windowHeight) {

    calculateCameraVectors();
    uploadData(windowWidth, windowHeight);
    changed_ = false;
}

void SchwarzschildCamera::setLockedMode(bool lock){
    lockedMode_ = lock;
    speedHistory_.clear();
    if (lockedMode_) {
       setViewDirTP(glm::vec2(VIEWTHETA, VIEWPHI));
    }
}

// set new view direction (xyz) and enforce limits on theta phi
void SchwarzschildCamera::setViewDirXYZ(glm::vec3 dir) {
    glm::vec3 locDir = dirXYZToCam(dir);
    setViewDirTP(XYZtoTP(locDir));
    changed_ = true;
}

// set new view direction (theta phi) and enforce limits on theta phi
void SchwarzschildCamera::setViewDirTP(glm::vec2 dir) {
    viewDirTP_ = dir;
    enforceViewDirLimits();
    glm::vec3 locDir = TPtoXYZ(dir);
    viewDirXYZ_ = dirCamToXYZ(locDir);
    changed_ = true;
}

// set new view position (xyz) and enforce limits on theta phi
void SchwarzschildCamera::setPosXYZ(glm::vec3 xyz, bool resetPrev) {
    setPosRTP(XYZtoRTP(xyz), resetPrev);
    changed_ = true;
}

// set new view position (rtp) and enforce limits on theta phi
void SchwarzschildCamera::setPosRTP(glm::vec3 rtp, bool resetPrev) {
    positionRTP_ = rtp;
    enforcePositionLimits();
    if (resetPrev) {
        positionXYZ_ = RTPtoXYZ(rtp);
        prevPositionXYZ_ = positionXYZ_;
    } else {
        prevPositionXYZ_ = positionXYZ_;
        positionXYZ_ = RTPtoXYZ(rtp);
    }
    changed_ = true;
}

void SchwarzschildCamera::processInput(GLFWwindow* window, float dt) {
    keyBoardInput(window, dt);
    mouseInput(window, dt);
}

void SchwarzschildCamera::keyBoardInput(GLFWwindow* window, float dt) {

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
    fov_ = std::clamp(fov_, 0.f, PI);


    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        velocityRTP_.z += stepSize;
        changed_ = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        velocityRTP_.z -= stepSize;
        changed_ = true;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        velocityRTP_.y += stepSize;
        changed_ = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        velocityRTP_.y -= stepSize;
        changed_ = true;
    }
}

void SchwarzschildCamera::mouseInput(GLFWwindow* window, float dt) {
    mouseInputLocked(window, dt);
}

void SchwarzschildCamera::storeConfig(boost::json::object& obj)
{
}

void SchwarzschildCamera::loadConfig(boost::json::object& obj)
{
}

glm::vec3 SchwarzschildCamera::dirCamToXYZ(glm::vec3 dir) const
{
    return getBase3() * dir;
}

glm::vec3 SchwarzschildCamera::dirXYZToCam(glm::vec3 dir) const
{
    return glm::inverse(getBase3()) * dir;
}

void SchwarzschildCamera::mouseInputLocked(GLFWwindow* window, float dt) {
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
        velocityRTP_.z = x - lastMouseX_;
        velocityRTP_.y = y - lastMouseY_;
    } else if (distance) {

        if (!distanceInit) {
            lastMouseY_ = y;
            distanceInit = true;
        }
        velocityRTP_.x = y - lastMouseY_;
    }
    else if (lookAround) {
        if (!lookAroundInit) {
            lastMouseX_ = x;
            lastMouseY_ = y;
            lookAroundInit = true;
        }

        // update camera orientation
        viewDirTP_.y += (x - lastMouseX_) * rotationSpeed_ * dt;
        viewDirTP_.x += -(y - lastMouseY_) * rotationSpeed_ * dt;
        setViewDirTP(viewDirTP_);

        lastMouseX_ = x;
        lastMouseY_ = y;

    }

    if (!rotation && friction_) {
        velocityRTP_.z *= 0.99f ;
        velocityRTP_.z = abs(velocityRTP_.z) < 0.000001f ? 0.f : velocityRTP_.z;

        velocityRTP_.y *= 0.99f;
        velocityRTP_.y = abs(velocityRTP_.y) < 0.000001f ? 0.f : velocityRTP_.y;
    }

    if (!distance && friction_) {
        velocityRTP_.x *= 0.99f;
        velocityRTP_.x = abs(velocityRTP_.x) < 0.000001f ? 0.f : velocityRTP_.x;
    }

    // update camera position
    float factor = 0.01f * dt;
    setPosRTP(positionRTP_ + velocityRTP_*factor, false);

    changed_ = true;
}

void SchwarzschildCamera::mouseInputFree(GLFWwindow* window) {
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
    viewDirTP_.y += (x - lastMouseX_) * rotationSpeed_;
    viewDirTP_.x += -(y - lastMouseY_) * rotationSpeed_;
    setViewDirTP(viewDirTP_);

    lastMouseX_ = x;
    lastMouseY_ = y;

    changed_ = true;
}

void SchwarzschildCamera::uploadData(int windowWidth, int windowHeight) {
    
    data_.camPos_ = positionXYZ_;
    
    glm::vec3 front = TPtoXYZ(viewDirTP_);
    glm::mat4 viewRot = glm::lookAt(glm::vec3(0.f), front, glm::vec3(0.f, 1.f, 0.f));
    data_.projectionInverse_ = glm::inverse(getProjectionMatrix((float)windowWidth / windowHeight) * viewRot);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraData), &data_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SchwarzschildCamera::bind() {
    glGenBuffers(1, &ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, CAMBINDING, ubo_);
}

void SchwarzschildCamera::calculateCameraVectors() {

    float sinTh = glm::sin(positionRTP_.y);
    float cosTh = glm::cos(positionRTP_.y);
    float sinPh = glm::sin(positionRTP_.z);
    float cosPh = glm::cos(positionRTP_.z);

    tau_ = glm::vec4(1.f, 0, 0, 0);

    front_ = -glm::vec4(
        0.f,
        sinTh*cosPh,
        cosTh,
        -sinTh*sinPh
    );

    right_ = glm::vec4(
        0.f,
        -sinPh,
        0, 
        -cosPh
    );

    up_ = -glm::vec4(
        0.f,
        cosTh*cosPh,
        -sinTh,
        -cosTh*sinPh
    );

}

void SchwarzschildCamera::enforceViewDirLimits(){
    viewDirTP_.x = glm::clamp(viewDirTP_.x, 0.01f, PI * 0.99f);
    viewDirTP_.y = glm::mod(viewDirTP_.y, 2.f * PI);
}

void SchwarzschildCamera::enforcePositionLimits() {
    positionRTP_.y = glm::clamp(positionRTP_.y, 0.01f, PI * 0.99f);
    positionRTP_.z = glm::mod(positionRTP_.z, 2.f * PI);
}

glm::vec3 SchwarzschildCamera::XYZtoRTP(glm::vec3 xyz) const {
    glm::vec3 rtp;
    rtp.x = glm::length(xyz);
    rtp.y = std::atan2(std::sqrt(xyz.z * xyz.z + xyz.x * xyz.x), xyz.y);
    rtp.z = std::atan2(-xyz.z, xyz.x);
    return rtp;
}

glm::vec3 SchwarzschildCamera::RTPtoXYZ(glm::vec3 rtp) const {
    glm::vec3 xyz;
    // calculate new position
    xyz.x = rtp.x * cos(rtp.z) * sin(rtp.y);
    xyz.y = rtp.x * cos(rtp.y);
    xyz.z = -rtp.x * sin(rtp.z) * sin(rtp.y);
    return xyz;
}

glm::vec2 SchwarzschildCamera::XYZtoTP(glm::vec3 xyz) const
{
    glm::vec2 dir;
    dir.x = std::atan2(std::sqrt(xyz.z * xyz.z + xyz.x * xyz.x), xyz.y);
    dir.y = std::atan2(-xyz.z, xyz.x);
    return dir;
}

glm::vec3 SchwarzschildCamera::TPtoXYZ(glm::vec2 yp) const
{
    glm::vec3 dir;
    dir.x = cos(yp.y) * sin(yp.x);
    dir.y = cos(yp.x);
    dir.z = -sin(yp.y) * sin(yp.x);
    return dir;
}

