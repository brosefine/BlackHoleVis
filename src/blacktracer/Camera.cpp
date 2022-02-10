#include <blacktracer/Camera.h>

/* ------------------------------------------------------------------------------------
* Source Code adapted from A.Verbraeck's Blacktracer Black-Hole Visualization
* https://github.com/annemiekie/blacktracer
* https://doi.org/10.1109/TVCG.2020.3030452
* ------------------------------------------------------------------------------------
*/

Camera::Camera(std::shared_ptr<Metric> metric, double theCam, double phiCam, double radfactor, double speedCam)
{
	metric_ = metric;
	
	theta = theCam;
	phi = phiCam;
	r = radfactor;
	speed = speedCam;

	bphi = 1;
	btheta = 0;
	br = 0;
	initforms();
}

/*
Camera::Camera(double theCam, double phiCam, double radfactor, double _br, double _btheta, double _bphi)
{
	theta = theCam;
	phi = phiCam;
	r = radfactor;

	bphi = _bphi;
	btheta = _btheta;
	br = _br;

	speed = metric_->calcSpeed(r, theta);
	initforms();
}
*/

void Camera::initforms()
{
	alpha = metric_->_alpha(this->r, this->theta);
	w = metric_->_w(this->r, this->theta);
	wbar = metric_->_wbar(this->r, this->theta);
	Delta = metric_->_Delta(this->r);
	ro = metric_->_ro(this->r, this->theta);
}

std::vector<float> Camera::getParamArray()
{
	std::vector<float> camera(7);
	camera[0] = speed;
	camera[1] = alpha;
	camera[2] = w;
	camera[3] = wbar;
	camera[4] = br;
	camera[5] = btheta;
	camera[6] = bphi;

	return camera;
}

