#include <objects/accretionDisk.h>

AccDisk::AccDisk(unsigned int binding)
	: ubo_(0)
	, binding_(binding) {

	data_.minRad_ = 4.f;
	data_.maxRad_ = 8.f;
	data_.rotation_ = 0.f;
	bind();
}

AccDisk::AccDisk(float min, float max, unsigned int binding)
	: ubo_(0)
	, binding_(binding) {

	data_.minRad_ = min;
	data_.maxRad_ = max;
	data_.rotation_ = 0.f;
	bind();
}

void AccDisk::uploadData() const {
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(AccDiskData), &data_);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void AccDisk::bind() {
	glGenBuffers(1, &ubo_);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(AccDiskData), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_, ubo_);
}
