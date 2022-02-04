#pragma once

/* ------------------------------------------------------------------------------------
* Source Code adapted from A.Verbraeck's Blacktracer Black-Hole Visualization
* https://github.com/annemiekie/blacktracer
* https://doi.org/10.1109/TVCG.2020.3030452
* ------------------------------------------------------------------------------------
*/

#include <blacktracer/Metric.h>

class BlackHole
{
public:
	double a;
	BlackHole(double afactor) {
		setA(afactor);
	};

	void setA(double afactor) {
		a = afactor;
		metric::setAngVel(afactor);
	}

	double getAngVel(double mass) {
		return mass*a;
	};
	~BlackHole(){};
};