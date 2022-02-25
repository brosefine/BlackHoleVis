# Kerr Vis

Application of Kerr [Black Hole visualization](https://github.com/annemiekie/blacktracer) by A. Verbraeck:
[Interactive Black-Hole Visualization](https://doi.org/10.1109/TVCG.2020.3030452)

## RenderModes
- SKY: a test shader which renders a cubemap
- COMPUTE: for testing compute shaders
- MAKEGRID: upload grid to gpu, expand it, and render to screen

## Controls
- Hold SHIFT and...
	- Drag w. left mouse button: rotate around black hole
	- Drag w. right mouse button: reduce or increase distance to black hole
- Hold CTRL and...
	- Drag w. left mouse button: rotate camera (change view direction but keep position)