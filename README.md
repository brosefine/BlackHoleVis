# BlackHoleVis
A project on black hole visualization

![](screenshots/screenshot_mix.png)

## Setup

### Dependencies
Dependencies are installed using [vcpkg](https://github.com/Microsoft/vcpkg).
- glm
- glad
- glfw
- imgui
- implot
- stb_image
- boost-json
- tinyobjloader
- cereal

### Windows
- [setup vcpkg](https://vcpkg.io/en/getting-started.html) (```vcpkg integrate install``` not required)
- install dependencies with vcpkg: 
	```vcpkg install glm:x64-windows glfw3:x64-windows stb:x64-windows glad:x64-windows imgui[opengl3-binding,glfw-binding]:x64-windows implot:x64-windows boost-json:x64-windows tinyobjloader:x64-windows cereal:x64-windows```
- open directory in CMake GUI
- select VS 2019
- add Entry:
	- name: ```CMAKE_TOOLCHAIN_FILE```
	- type: ```PATH```
	- value: ```[path to vcpkg]/scripts/buildsystems/vcpkg.cmake```
- Configure & Generate
- Open Project to open solution in VS

### Linux (TODO)

## Contents
- [App 1](app/BlackHoleVis_1/README.md)
- [App 2](app/BlackHoleVis_2/README.md)
- [App 3](app/BlackHoleVis_3/README.md)

## References on Black Hole Visualization
- [LearnOpenGL](https://learnopengl.com/)
- [Randonels Starless](https://github.com/rantonels/starless)
- [Coding Train - Visualizing a Black Hole](https://www.youtube.com/watch?v=Iaz9TqYWUmA)
- [Real-time High-Quality Rendering of Non-Rotating Black Holes](https://github.com/ebruneton/black_hole_shader)
