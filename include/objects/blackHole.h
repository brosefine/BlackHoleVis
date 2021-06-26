#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

/*** Units ***
* units were adapted to make the code a bit cleaner
* mass: 1 = 1 sun mass = 2e30 kg
* distance: 1 = 10e8 m (-> speed of light becomes 3)
*/

class BlackHole {
	struct BlackHoleData {
		glm::vec3 position_;
		float mass_;
		float radius_;	// schwarzschild-radius
	};

public:
	BlackHole() {};
	BlackHole(unsigned int binding);
	BlackHole(glm::vec3 pos, float mass, unsigned int binding);

	void uploadData() const;
	unsigned int getUbo() const { return ubo_; }
	unsigned int getBinding() const { return binding_; }
	float getRadius() const { return data_.radius_; }
	float getMass() const { return data_.mass_; }

	void setMass(float mass) { 
		data_.mass_ = mass;
		calcRadius();
	}

private:

	unsigned int ubo_;
	unsigned int binding_;
	
	BlackHoleData data_;

	void bind();
	void calcRadius();
};