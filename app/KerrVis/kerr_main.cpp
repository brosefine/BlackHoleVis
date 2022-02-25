#include <kerr_app.h>
#include <helpers/RootDir.h>
#include <blacktracer/Grid.h>

#include <iostream>
#include <memory>

int main() {

	KerrApp app(800, 600);
	app.renderLoop();

	
	return 0;

}

