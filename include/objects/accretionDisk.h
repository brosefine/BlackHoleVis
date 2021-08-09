#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class AccDisk {
	struct AccDiskData {
		float minRad_;
		float maxRad_;
		float rotation_;
	};

public:
	AccDisk() {};
	AccDisk(unsigned int binding);
	AccDisk(float min, float max, unsigned int binding);

	void uploadData() const;
	unsigned int getUbo() const { return ubo_; }
	unsigned int getBinding() const { return binding_; }
	float getMinRad() const { return data_.minRad_; }
	float getMaxRad() const { return data_.maxRad_; }

	void setRad(float min, float max) {
		data_.minRad_ = min;
		data_.maxRad_ = max;
	}

	void setRotation(float rot) {
		data_.rotation_ = rot;
	}

private:

	unsigned int ubo_;
	unsigned int binding_;

	AccDiskData data_;

	void bind();
};