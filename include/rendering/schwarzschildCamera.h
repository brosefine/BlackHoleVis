#pragma once

#include <rendering/UBOCamera.h>

#include <boost/json.hpp>


#include <vector>
#include <deque>
#include <algorithm>

class SchwarzschildCamera : public UBOCamera {
	inline static float PI = glm::pi<float>();
	inline static float HALFPI = glm::half_pi<float>();
	inline static float VIEWPHI = HALFPI;
	inline static float VIEWTHETA = HALFPI;
	inline static float FOV = glm::radians(45.f);
	inline static float NEAR = 1.f;
	inline static float FAR = 100.f;
	inline static int SMOOTHSPEED = 10;

public:

	SchwarzschildCamera();
	SchwarzschildCamera(glm::vec3 pos);

	glm::mat4 getProjectionMatrix(float aspect, float near = NEAR, float far = FAR);
	CameraData getData() const { return data_; }
	glm::vec3 getPositionXYZ() const { return positionXYZ_; }
	glm::vec3 getPositionRTP() const { return positionRTP_; }
	glm::vec4 getFront() const { return front_; }
	glm::vec4 getUp() const { return up_; }
	glm::vec4 getRight() const { return right_; }
	glm::mat3 getBase3() const;
	glm::mat3 getFidoBase3() const;
	glm::mat4 getBase4() const;
	glm::mat4 getFidoBase4() const;
	glm::vec3 getCurrentVelXYZ() const { return positionXYZ_ - prevPositionXYZ_; }
	glm::mat3 getLookAtRot() const;
	float getFov() const { return fov_; }
	float getAvgSpeed() const;
	float getAvgSpeed(float newSpeed);
	float getTheta() const { return positionRTP_.y; }
	float getPhi() const { return positionRTP_.z; }
	float getSpeedScale() const { return speedScale_; }
	bool isLocked() const { return lockedMode_; }
	
	glm::mat4 getBoostLocal(float dt);
	glm::mat4 getBoostGlobal(float dt);
	glm::mat4 getBoostLocalFido(float dt);
	glm::mat4 getBoostFromVel(glm::vec3 velocity) const;
	glm::mat4 getBoostFromVel(glm::vec3 dir, float speed) const;

	bool hasChanged() const { return changed_; }
	void update(int windowWidth, int windowHeight) override;

	void setSpeed(float speed) { translationSpeed_ = speed; }
	void setViewDirXYZ(glm::vec3 dir);
	void setViewDirTP(glm::vec2 dir);
	void setPosXYZ(glm::vec3 xyz, bool resetPrev = true);
	void setPosRTP(glm::vec3 rtp, bool resetPrev = true);
	void setLockedMode(bool lock);
	void setFriction(bool f) { friction_ = f; }
	void setSpeedScale(float s) { speedScale_ = std::clamp(s, 0.f, 1.f); }

	void processInput(GLFWwindow* window, float dt);
	void keyBoardInput(GLFWwindow *window, float dt);
	void mouseInput(GLFWwindow* window, float dt);

	void storeConfig(boost::json::object& obj);
	void loadConfig(boost::json::object& obj);

private:

	void uploadData(int windowWidth, int windowHeight);

	// camera

	void calculateCameraVectors();
	void enforceViewDirLimits();
	void enforcePositionLimits();
	// convert positions
	glm::vec3 XYZtoRTP(glm::vec3 xyz) const;
	glm::vec3 RTPtoXYZ(glm::vec3 rtp) const;
	glm::vec2 XYZtoTP(glm::vec3 xyz) const;
	glm::vec3 TPtoXYZ(glm::vec2 yp) const;
	// convert directions
	glm::vec3 dirCamToXYZ(glm::vec3 dir) const;
	glm::vec3 dirXYZToCam(glm::vec3 dir) const;

	void mouseInputLocked(GLFWwindow* window, float dt);
	void mouseInputFree(GLFWwindow* window);


	// RTP = Rad, Theta, Phi in radians
	// XYZ = Cartesian
	glm::vec3 positionRTP_;
	glm::vec3 velocityRTP_;
	glm::vec3 positionXYZ_;
	glm::vec3 prevPositionXYZ_;
	// view direction in global xyz coordinates
	glm::vec3 viewDirXYZ_;
	// view direction in local yaw and pitch
	glm::vec2 viewDirTP_;
	glm::vec4 tau_;
	glm::vec4 up_;
	glm::vec4 front_;
	glm::vec4 right_;

	float fov_;

	float translationSpeed_;
	float translationFactor_;
	float rotationSpeed_;

	// for calculating an average speed over a number of frames
	std::deque<float> speedHistory_;
	float speedScale_;
	double speedWeights_;
	double speedSum_;

	double lastMouseX_;
	double lastMouseY_;
	bool mouseInitialized_;

	bool mouseMotion_;
	// switch between two camera modes:
	// 0 = free mode, wasd = motion, mouse = rotate camera
	// 1 = locked mode, left mouse = rotate around object, right mouse = distance
	bool lockedMode_;
	bool friction_;
};