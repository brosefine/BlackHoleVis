#include <kerr_app.h>
#include <helpers/RootDir.h>
#include <blacktracer/Grid.h>

#include <iostream>
#include <memory>

int main() {

	KerrApp app(800, 600);
	app.renderLoop();

	/*
	std::string filename = "rayTraceLvl1to10Pos10.5_0.5_0Spin0.999_sp_.grid";
	std::cout << "Loading Grid file " << filename << std::endl;

	std::shared_ptr<Grid> grid1 = std::make_shared<Grid>();
	Grid::loadFromFile(grid1, filename);

	std::cout << "Grid data: MAXLEVEL: "
		<< grid1->MAXLEVEL << ", N: "
		<< grid1->N << ", M: "
		<< grid1->M << std::endl;

	grid1->hasher.writeToFile(std::string(ROOT_DIR "resources/grids/" + filename + ".txt"));

	GridProperties props;
	
	std::shared_ptr<Grid> grid2 = std::make_shared<Grid>();
	Grid::makeGrid(grid2, props);
	Grid::saveToFile(grid2);
	grid2->hasher.writeToFile(std::string(ROOT_DIR "resources/grids/" + Grid::getFileNameFromConfig(props) + ".txt"));

	std::shared_ptr<Grid> grid3 = std::make_shared<Grid>();
	Grid::makeGrid(grid3, props);
	grid3->hasher.writeToFile(std::string(ROOT_DIR "resources/grids/" + Grid::getFileNameFromConfig(props) + "_loaded.txt"));

	props.blackHole_a_ = 0.9;
	std::shared_ptr<Grid> grid4 = std::make_shared<Grid>(props);
	grid4->printGridCam(0);
	grid4->printGridCam(1);
	grid4->printGridCam(2);
	grid4->printGridCam(3);
	*/
	return 0;

}

