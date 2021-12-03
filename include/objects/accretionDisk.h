#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

const int NUMPARTICLES = 10;

class AccDisc {
	struct AccDiscData {
		float minRad_;
		float maxRad_;
		float rotation_;
	};

public:
	AccDisc() {};
	AccDisc(unsigned int binding);
	AccDisc(float min, float max, unsigned int binding);

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

	AccDiscData data_;

	void bind();
};


// Accretion disk as presented by e.bruneton
// https://github.com/ebruneton/black_hole_shader
class ParticleDisc {
	struct ParticleDiscData {
		glm::vec4 params_;	// density, opacity, temperature, numparticles
		glm::vec4 size_;	// rmin, rmax, irmin, irmax
		glm::vec4 particles_[NUMPARTICLES];
	};

public:
	ParticleDisc() {};
	ParticleDisc(unsigned int binding);
	ParticleDisc(float min, float max, unsigned int binding);

	void uploadData() const;
	unsigned int getUbo() const { return ubo_; }
	unsigned int getBinding() const { return binding_; }

	// return vec2 containing {minRad, maxRad}
	glm::vec2 getRad() const { return { data_.size_.x, data_.size_.y }; }
	float minRad() const { return data_.size_.x; }
	float maxRad() const { return data_.size_.y; }
	float getDensity() const { return data_.params_.x; }
	float getOpacity() const { return data_.params_.y; }
	float getTemp() const { return data_.params_.z; }

	void setDensity(float dens) { data_.params_.x = dens; }
	void setOpacity(float op) { data_.params_.y = op; }
	void setTemp(float temp) { data_.params_.z = temp; }
	void setRad(float min, float max) {
		data_.size_.x = min;
		data_.size_.y = max;
		data_.size_.z = 1.f/min;
		data_.size_.w = 1.f/max;
	}
	
private:

	unsigned int ubo_;
	unsigned int binding_;

	ParticleDiscData data_;

	void bind();
	void generateParticles();
};