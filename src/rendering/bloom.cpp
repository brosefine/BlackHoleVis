#include <rendering/bloom.h>
#include <helpers/json_helper.h>

Bloom::Bloom(int width, int height, int level)
	: intensity_(0.5f)
	, exposure_(1.f)
	, highContrast_(false)
	, maxLevels_(level)
	, width_(width)
	, height_(height)
{
	initLevels();
	initFBOS();
	initShaders();
}

void Bloom::begin() {
	source_->bindFBO();
	glViewport(0, 0, width_, height_);
}

void Bloom::render(int fboId) {
	source_->generateMipMap();
	// bloom pass
	glActiveTexture(GL_TEXTURE0);
	source_->bind();
	
	for (int i = 1; i < levelSizes_.size(); ++i) {
		//filters_->setParam(GL_TEXTURE_BASE_LEVEL, 0);
		filters_->setMipMapRenderLevel(i);
		filters_->bindFBO();

		glViewport(0, 0, levelSizes_.at(i).x, levelSizes_.at(i).y);
		bloomShader_->use();
		bloomShader_->setUniform("texDimLevel", glm::vec3(levelSizes_.at(i), i));
		quad_.draw(GL_TRIANGLES);
	}
	
	// upsample pass
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	upsampleShader_->use();
	for (int i = levelSizes_.size() - 2; i >= 1; --i) {
		filters_->setMipMapRenderLevel(i);
		filters_->bindFBO();
		filters_->bind();
		glViewport(0, 0, levelSizes_.at(i).x, levelSizes_.at(i).y);
		upsampleShader_->setUniform("texDimLevel", glm::vec3(levelSizes_.at(i+1), i+1));
		quad_.draw(GL_TRIANGLES);
	}
	glDisable(GL_BLEND);
	
	// final pass
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glActiveTexture(GL_TEXTURE0);
	source_->bind();
	glActiveTexture(GL_TEXTURE1);
	//filters_->setParam(GL_TEXTURE_BASE_LEVEL, 0);
	filters_->bind();
	glViewport(0, 0, width_, height_);
	renderShader_->use();
	renderShader_->setUniform("texDim", glm::vec2(width_, height_));
	renderShader_->setUniform("intensity", intensity_);
	renderShader_->setUniform("exposure", exposure_);
	renderShader_->setUniform("high_contrast", highContrast_);

	quad_.draw(GL_TRIANGLES);
}

void Bloom::reload() {
	upsampleShader_->reload();
	bloomShader_->reload();
	renderShader_->reload();
}

void Bloom::resize(int width, int height) {
	width_ = width;
	height_ = height;

	source_->resize(width, height);
	source_->generateMipMap();
	filters_->resize(width, height);
	filters_->generateMipMap();

	initLevels();
}

void Bloom::setLevel(int level){
	maxLevels_ = level;
	initLevels();
}

void Bloom::storeConfig(boost::json::object& obj){
	obj["intensity"] = intensity_;
	obj["exposure"] = exposure_;
	obj["highContrast"] = highContrast_;
	obj["maxLevels"] = maxLevels_;
	obj["width"] = width_;
	obj["height"] = height_;
}

void Bloom::loadConfig(boost::json::object& obj){
	jhelper::getValue(obj, "intensity", intensity_);
	jhelper::getValue(obj, "exposure", exposure_);
	jhelper::getValue(obj, "highContrast", highContrast_);
	jhelper::getValue(obj, "maxLevels", maxLevels_);
	jhelper::getValue(obj, "width", width_);
	jhelper::getValue(obj, "height", height_);

	setLevel(maxLevels_);
	resize(width_, height_);
}

void Bloom::initShaders() {
	upsampleShader_ = std::make_shared<Shader>("squad.vs", "bloom_upsample.frag");
	bloomShader_ = std::make_shared<Shader>("squad.vs", "bloom.frag");
	renderShader_ = std::make_shared<Shader>("squad.vs", "bloom_render.frag");
}

void Bloom::initFBOS() {
	source_ = std::make_shared<FBOTexture>(width_, height_);
	source_->generateMipMap();

	filters_ = std::make_shared<FBOTexture>(width_, height_);
	filters_->generateMipMap();

	std::vector<std::pair<GLenum, GLint>> texParameters{
			{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
			{GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
			{GL_TEXTURE_MAG_FILTER, GL_LINEAR}
	};
	source_->setParam(texParameters);
	filters_->setParam(texParameters);
}

void Bloom::initLevels() {
	levelSizes_.clear();
	int level = 0;
	int w = width_, h = height_;
	while (h > 2 && w > 2 && level < maxLevels_) {
		levelSizes_.push_back({ w,h });
		w = floor(w / 2.f);
		h = floor(h / 2.f);
		++level;
	}
}
