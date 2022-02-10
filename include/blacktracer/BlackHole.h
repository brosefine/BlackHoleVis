#pragma once

/* ------------------------------------------------------------------------------------
* Source Code adapted from A.Verbraeck's Blacktracer Black-Hole Visualization
* https://github.com/annemiekie/blacktracer
* https://doi.org/10.1109/TVCG.2020.3030452
* ------------------------------------------------------------------------------------
*/

#include <blacktracer/MetricClass.h>
#include <memory>

class BlackHole
{
public:
	double a;
	std::shared_ptr<Metric> metric_;
	BlackHole(std::shared_ptr<Metric> metric, double afactor) 
		: metric_(metric)
		, a(afactor){
		metric_->setAngVel(a);
	};

	void setA(double afactor) {
		a = afactor;
		metric_->setAngVel(a);
	}

	double getAngVel(double mass) {
		return mass*a;
	};
	~BlackHole(){};
};