#include <kerr_app.h>
#include <helpers/RootDir.h>
#include <blacktracer/Grid.h>

#include <iostream>

int main() {

	//KerrApp app(800, 600);
	//app.renderLoop();

	std::string filename = "rayTraceLvl1to10Pos10.5_0.5_0Spin0.999_sp_.grid";
	std::cout << "Loading Grid file " << filename << std::endl;

	Grid grid;
	Grid::loadFromFile(grid, filename);

	std::cout << "Grid data: MAXLEVEL: "
		<< grid.MAXLEVEL << ", N: "
		<< grid.N << ", M: "
		<< grid.M << std::endl;

	grid.hasher.writeToFile(std::string(ROOT_DIR "resources/grids/" + filename + ".txt"));

	return 0;

}

