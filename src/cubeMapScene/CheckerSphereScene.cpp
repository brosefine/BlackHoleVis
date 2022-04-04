#include <cubeMapScene/CheckerSphereScene.h>
#include <gui/gui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <helpers/uboBindings.h>



CheckerSphereScene::CheckerSphereScene()
	: CubeMapScene()
	, sphereColor_(0.f)
	, spherePos_(0.f)
	, sphereScale_(1.f)
	, drawSky_(true){}

CheckerSphereScene::CheckerSphereScene(int size)
	: CubeMapScene(size)
	, sphereMesh_(std::make_shared<Mesh>("sphere.obj"))
	, cubeMesh_(std::make_shared<Mesh>("cube.obj"))
	, sphereColor_(1.f)
	, spherePos_(5.f, 0.f, 0.f)
	, sphereScale_(1.f)
	, cubePos_(-5.f, 0.f, 0.f)
	, cubeScale_(1.f)
	, drawSky_(true)
{
	loadShaders();
	loadTextures();
}

void CheckerSphereScene::render(glm::vec3 camPos, float dt)
{
	updateCameras(camPos);

	glBindFramebuffer(GL_FRAMEBUFFER, fboID_);
	glViewport(0, 0, envMap_->getWidth(), envMap_->getHeight());

	glEnable(GL_DEPTH_TEST);
	for (unsigned int i = 0; i < 6; ++i) {

		GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face, envMap_->getTexId(), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, face, depthMap_->getTexId(), 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		envCameras_.at(i)->use(envMap_->getWidth(), envMap_->getHeight());

		sphereShader_->use();
		sphereShader_->setUniform("modelMatrix", glm::translate(spherePos_) * glm::scale(glm::vec3(sphereScale_)) * glm::mat4(1));

		sphereShader_->setUniform("mesh_color", sphereColor_);

		sphereMesh_->draw(GL_TRIANGLES);

		cubeShader_->use();
		cubeShader_->setUniform("modelMatrix", glm::translate(cubePos_) * glm::scale(glm::vec3(cubeScale_)) * glm::mat4(1));

		cubeMesh_->draw(GL_TRIANGLES);

		if (drawSky_) {

			glActiveTexture(GL_TEXTURE0);
			skyTexture_->bind();
			skyShader_->use();
			quad_.draw(GL_TRIANGLES);
		}

	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
}

void CheckerSphereScene::renderGui()
{
	ImGui::Text("Checker Sphere Scene Settings");
	if (ImGui::CollapsingHeader("Sphere")) {
		ImGui::Indent();
		ImGui::SliderFloat("Sphere Size", &sphereScale_, 0.1f, 10);
		ImGui::SliderFloat3("Sphere Pos", glm::value_ptr(spherePos_), -50.f, 50.f);
		ImGui::SliderFloat3("Sphere Color", glm::value_ptr(sphereColor_), 0.f, 1.f);
		ImGui::Unindent();
	}
	if (ImGui::CollapsingHeader("Cube")) {
		ImGui::Indent();
		ImGui::SliderFloat("Cube Size", &cubeScale_, 0.1f, 10);
		ImGui::SliderFloat3("Cube Pos", glm::value_ptr(cubePos_), -50.f, 50.f);
		ImGui::Unindent();
	}
	ImGui::Checkbox("Draw Sky", &drawSky_);

	if (ImGui::Button("Reload Shaders"))
		reloadShaders();
}

void CheckerSphereScene::loadTextures()
{
	std::vector<std::string> skyFaces{
		"grid/right.png",
		"grid/right.png",
		"grid/top.png",
		"grid/bottom.png",
		"grid/right.png",
		"grid/right.png"
	};
	skyTexture_ = std::make_shared<CubeMap>(skyFaces);
}

void CheckerSphereScene::loadShaders()
{
	skyShader_ = std::make_shared<Shader>("cubeMapScene/sky.vs", "cubeMapScene/sky.fs");
	sphereShader_ = std::make_shared<Shader>("cubeMapScene/checkerSphereMesh.vs", "cubeMapScene/checkerSphereMesh.fs");
	cubeShader_ = std::make_shared<Shader>("cubeMapScene/checkerSphereMesh.vs", "cubeMapScene/cubeMesh.fs");
	reloadShaders();
}

void CheckerSphereScene::reloadShaders()
{
	sphereShader_->reload();
	cubeShader_->reload();
	skyShader_->reload();
	skyShader_->setBlockBinding("camera", CAMBINDING);
	sphereShader_->setBlockBinding("camera", CAMBINDING);
	cubeShader_->setBlockBinding("camera", CAMBINDING);
}
