# BlackHoleVis
Computer Science Master's Project at the FSU Jena 

![](screenshots/Screenshot_starless.png)

## Setup

### Dependencies
Dependencies were installed using [vcpkg](https://github.com/Microsoft/vcpkg).
- glm
- glad
- glfw
- imgui
- implot
- stb_image

### Windows
- [setup vcpkg](https://vcpkg.io/en/getting-started.html) (```vcpkg integrate install``` not required)
- install dependencies with vcpkg: 
	```vcpkg install glm:x64-windows glfw3:x64-windows stb:x64-windows glad:x64-windows imgui[opengl3-binding,glfw-binding]:x64-windows implot:x64-windows```
- open directory in CMake GUI
- select VS 2019
- add Entry:
	- name: ```CMAKE_TOOLCHAIN_FILE```
	- type: ```PATH```
	- value: ```[path to vcpkg]/scripts/buildsystems/vcpkg.cmake```
- Configure & Generate
- Open Project to open solution in VS

### Linux (TODO)

## Controls
- WASD: move camera
- SHIFT: move faster
- Mouse move & right mouse click: rotate camera

## Sources
### Images and Textures
- Alternative Skybox generated with [space-3d](https://wwwtyro.github.io/space-3d/#animationSpeed=1&fov=80&nebulae=true&pointStars=true&resolution=1024&seed=3wq0xhr2fwu8&stars=true&sun=false) by Rye Terrell
- Skybox panorama by [ESO](https://www.eso.org/public/germany/images/eso0932a/)
### Black Hole Visualization
- [LearnOpenGL](https://learnopengl.com/)
- [Randonels Starless](https://github.com/rantonels/starless)
- [Coding Train - Visualizing a Black Hole](https://www.youtube.com/watch?v=Iaz9TqYWUmA)
- [Real-time High-Quality Rendering of Non-Rotating Black Holes](https://github.com/ebruneton/black_hole_shader)
