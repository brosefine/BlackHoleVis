#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <iostream>

#include <rendering/glBoilerplate.h>
#include <rendering/shader.h>
#include <rendering/quad.h>
#include <rendering/camera.h>
#include <objects/blackHole.h>

float t0 = 0.f, dt = 0.f;
Shader blackHoleShader;
BlackHole blackHole;
Camera cam;

bool show_demo_window = true;

void processKeyboardInputs(GLFWwindow* window) {

	// close window with escape
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		blackHoleShader.reload();
}

int main() {

	GLFWwindow *windowPtr = glBoilerplate::init(windowWidth, windowHeight);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(windowPtr, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//Shader blackHoleShader("blackHole.vert", "blackHoleTest.frag");
	blackHoleShader = Shader("blackHole.vert", "newton.frag");
	Mesh quad(quadPostions, quadUVs, quadIndices);

	unsigned int blackHoleBinding = 1;
	blackHole = BlackHole({ 0.f, 0.f, 0.f }, 1.0e6, blackHoleBinding);
	blackHole.uploadData();
	blackHoleShader.use();
	blackHoleShader.setBlockBinding("blackHole", blackHoleBinding);

	std::cout << "Radius: " << blackHole.getRadius() << std::endl;

	// place camera at a distance of 10 black hole (schwarzschild-)radii
	cam = Camera({ 0.f, 0.f, -10*blackHole.getRadius() }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f });
	// set camera speed to a value appropriate to the scene's scale
	cam.setSpeed(5*blackHole.getRadius());

	while (!glfwWindowShouldClose(windowPtr)) {

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		if(show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
		ImGui::Render();

		float now = glfwGetTime();
		dt = now - t0;
		t0 = now;

		processKeyboardInputs(windowPtr);
		cam.keyBoardInput(windowPtr, dt);
		cam.mouseInput(windowPtr);

		glClearColor(0.5, 0.5, 0.5, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		blackHoleShader.use();

		if (cam.hasChanged()) {
			blackHoleShader.setUniform("cameraPos", cam.getPosition());
			blackHoleShader.setUniform("projectionViewInverse", 
				glm::inverse(cam.getProjectionMatrix((float)windowWidth / windowHeight) * cam.getViewMatrix()));
		}
		quad.draw(GL_TRIANGLES);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
		glfwPollEvents();
		glfwSwapBuffers(windowPtr);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(windowPtr);
	glfwTerminate();
	return 0;

}

