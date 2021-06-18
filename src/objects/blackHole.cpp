#include <objects/blackHole.h>

BlackHole::BlackHole(unsigned int binding) 
	: ubo_(0)
	, binding_(binding) {

	data_.position_ = glm::vec3{ 0, 0, 0 };
	data_.mass_ = 1.f;
	calcRadius();
	bind();
}

BlackHole::BlackHole(glm::vec3 pos, float mass, unsigned int binding)
	: ubo_(0)
	, binding_(binding) {

	data_.position_ = pos;
	data_.mass_ = mass;
	calcRadius();
	bind();
}
void BlackHole::uploadData() const {
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(BlackHoleData), &data_);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void BlackHole::bind() {

	glGenBuffers(1, &ubo_);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(BlackHoleData), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_, ubo_);
}

void BlackHole::calcRadius() {
	// short version of r_s = 2 * (G*M)/c^2
	// with units used as described in 'units'
	data_.radius_ = data_.mass_ * 3.0e-5;
}
