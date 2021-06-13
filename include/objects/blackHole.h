#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class BlackHole {
	struct BlackHoleData {
		glm::vec3 position_;
		float mass_;
	};

public:
	BlackHole(unsigned int binding);
	BlackHole(glm::vec3 pos, float mass, unsigned int binding);

	void uploadData() const;
	unsigned int getUbo() const { return ubo_; }
	unsigned int getBinding() const { return binding_; }

private:

	unsigned int ubo_;
	unsigned int binding_;
	
	BlackHoleData data_;

	void bind();
};