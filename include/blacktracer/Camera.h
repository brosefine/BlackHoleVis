#pragma once

/* ------------------------------------------------------------------------------------
* Source Code adapted from A.Verbraeck's Blacktracer Black-Hole Visualization
* https://github.com/annemiekie/blacktracer
* https://doi.org/10.1109/TVCG.2020.3030452
* ------------------------------------------------------------------------------------
*/

#include <blacktracer/MetricClass.h>

#include <vector>
#include <memory>

class Camera
{
public:
	// position
	double theta, phi, r; 
	// direction of motion
	double speed, br, btheta, bphi;

	double alpha, w, wbar, Delta, ro;

	std::shared_ptr<Metric> metric_;


	Camera(){};

	Camera(std::shared_ptr<Metric> metric, double theCam, double phiCam, double radfactor, double speedCam);;

	//Camera(double theCam, double phiCam, double radfactor, double _br, double _btheta, double _bphi);;

	void initforms();;

	double getDistFromBH(float mass) {
		return mass*r;
	};

	/// <summary>
	/// Pack all camera parameters into an array:
	/// speed,alpha,w,wbar,br,btheta,bphi
	/// </summary>
	/// <returns></returns>
	std::vector<float> getParamArray();;

	~Camera(){};
};


