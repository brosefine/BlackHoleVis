#include <objects/accretionDisk.h>

#include <random>

#include <glm/gtc/constants.hpp>



#pragma region simple disk

AccDisc::AccDisc(unsigned int binding)
	: ubo_(0)
	, binding_(binding) {

	data_.minRad_ = 4.f;
	data_.maxRad_ = 8.f;
	data_.rotation_ = 0.f;
	bind();
}

AccDisc::AccDisc(float min, float max, unsigned int binding)
	: ubo_(0)
	, binding_(binding) {

	data_.minRad_ = min;
	data_.maxRad_ = max;
	data_.rotation_ = 0.f;
	bind();
}

void AccDisc::uploadData() const {
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(AccDiscData), &data_);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void AccDisc::bind() {
	glGenBuffers(1, &ubo_);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(AccDiscData), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_, ubo_);
}

#pragma endregion

#pragma region particle disk

// Particle disk with default values
ParticleDisc::ParticleDisc(unsigned int binding)
	: ubo_(0)
	, binding_(binding) 
{
	
	data_.params_.x = 1.f; // density
	data_.params_.y = 1.f; // opacity
	data_.params_.z = 1000.f; // temperature in K
	data_.params_.w = NUMPARTICLES;
	
	data_.size_.x = 3.f;
	data_.size_.y = 12.f;
	data_.size_.z = 1.f / data_.size_.x;
	data_.size_.w = 1.f / data_.size_.y;

	generateParticles();
	bind();
	uploadData();

}

ParticleDisc::ParticleDisc(float min, float max, unsigned int binding)
	: ubo_(0)
	, binding_(binding)
{
	data_.params_.x = 1.f; // density
	data_.params_.y = 1.f; // opacity
	data_.params_.z = 1000.f; // temperature in K
	data_.params_.w = NUMPARTICLES;

	data_.size_.x = glm::min(3.f, min);
	data_.size_.y = max;
	data_.size_.z = 1.f / data_.size_.x;
	data_.size_.w = 1.f / data_.size_.y;

	generateParticles();
	bind();
	uploadData();
}

void ParticleDisc::uploadData() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ParticleDiscData), &data_);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ParticleDisc::bind()
{
	glGenBuffers(1, &ubo_);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ParticleDiscData), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_, ubo_);
}

void ParticleDisc::generateParticles()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0.f, 1.f);
	float rstep = (maxRad() - minRad()) / (float)NUMPARTICLES;
	float dRdPhiSteps = 1000.f, dx = 1.f/dRdPhiSteps;
	for (int i = 0; i < NUMPARTICLES; ++i) {
		float inner_r = minRad() + i * rstep;
		float inner_u = 1.f / inner_r;
		// compute an outer radius slightly larger than inner by a random factor
		float randScale = 0.1f * dis(gen);
		float outer_r = inner_r * (1.f + randScale) / (1.f - randScale);
		float outer_u = 1.f / outer_r;
		float phi0 = 2.f * glm::pi<float>() * dis(gen);

		// compute change of radius with phi
		float med_u = 1.f - inner_u - outer_u;
		float kappa2 = (outer_u - inner_u) / (med_u - inner_u);
		float dRdPhi = 0.f;
		for (int j = 0; j < dRdPhiSteps; ++j) {
			float x = (j + 0.5f) / dRdPhiSteps;
			dRdPhi += dx / glm::sqrt((1.f - x * x) * (1.f - kappa2 * x * x));
		}
		dRdPhi = glm::pi<float>() * glm::sqrt(med_u - inner_u) / (4 * dRdPhi);

		data_.particles_[i] = glm::vec4(inner_u, outer_u, phi0, dRdPhi);
	}
}

#pragma endregion