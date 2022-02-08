#pragma once

/* ------------------------------------------------------------------------------------
* Source Code adapted from A.Verbraeck's Blacktracer Black-Hole Visualization
* https://github.com/annemiekie/blacktracer
* https://doi.org/10.1109/TVCG.2020.3030452
* ------------------------------------------------------------------------------------
*/

#include <blacktracer/Const.h>
#include <blacktracer/Code.h>

#include <glm/glm.hpp>

#include <iostream>
#include <vector>
#include <string>

#define rVar var[0]			// r, phi, theta give position
#define thetaVar var[1]
#define phiVar var[2]
#define pRVar var[3]		// p_r, p_theta are components of the 4-momentum
#define pThetaVar var[4]

#define drdz varOut[0]
#define dtdz varOut[1]
#define dpdz varOut[2]
#define dprdz varOut[3]
#define dptdz varOut[4]

#define SAFETY 0.9
#define PGROW -0.2
#define PSHRNK -0.25
#define ERRCON 1.89e-4
#define MAXSTP 1000
#define TINY 1.0e-30
#define ADAPTIVE 5.0


// Simple Singleton Example by Martin York
// https://stackoverflow.com/a/1008289
class Metric {
public:
	static Metric& getInstance() {
		static Metric instance;
		return instance;
	}

	double a() { return a_; }
	double asq() { return asq_; }

	void setAngVel(double afactor)
	{
		a_ = afactor;
		asq_ = afactor * afactor;
	}

	// square function
	double sq(double x) {
		return x * x;
	}

	double sq3(double x) {
		return x * x * x;
	}

	double _Delta(double r) {
		return sq(r) - 2. * r + asq_;
	};

	double _Sigma(double r, double theta) {
		//         ~1~         2                3                  4
		//return sqrt(sq(sq(r) + PAsq) - PAsq*_Delta(r) * sq(sin(theta)));
		double v2 = sq(sq(r) + asq_);
		double v3 = asq_ * _Delta(r);
		double v4 = sq(sin(theta));
		double result = sqrt(v2 - v3 * v4);
		return result;
	};

	double _w(double r, double theta) {
		return 2. * a_ * r / sq(_Sigma(r, theta));
	};

	double _ro(double r, double theta) {
		return sqrt(sq(r) + asq_ * sq(cos(theta)));
	};

	double _rosq(double r, double theta) {
		return sq(r) + asq_ * sq(cos(theta));
	};

	double _wbar(double r, double theta) {
		return _Sigma(r, theta) * sin(theta) / _ro(r, theta);
	};

	double _alpha(double r, double theta) {
		//          1                     2             3
		//return _ro(r, theta) * sqrt(_Delta(r)) / _Sigma(r, theta);
		double v1 = _ro(r, theta);
		double v2 = sqrt(_Delta(r));
		double v3 = _Sigma(r, theta);
		double result = v1 * v2 / v3;
		return result;
	};

	double _P(double r, double b) {
		return sq(r) + asq_ - a_ * b;
	}

	double _R(double r, double theta, double b, double q) {
		//           1             2         3           
		//return sq(_P(r, b)) - _Delta(r)*(sq((b - PA)) + q);
		double v1 = sq(_P(r, b));
		double v2 = _Delta(r);
		double v3 = (sq((b - a_)) + q);
		double result = v1 - (v2 * v3);
		//std::cout << "v1=" << v1 << " v2=" << v2 << " v3=" << v3 << " _R: " << result << std::endl;
		return result;
	};

	double _BigTheta(double r, double theta, double b, double q) {
		return q - sq(cos(theta)) * (sq(b) / sq(sin(theta)) - asq_);
	};

	double calcSpeed(double r, double theta) {
		double omega = 1. / (a_ + pow(r, 1.5));
		double sp = _wbar(r, theta) / _alpha(r, theta) * (omega - _w(r, theta));
		return sp;
	}

	double findMinGoldSec(double theta, double bval, double qval, double ax, double b, double tol) {
		double gr = (sqrt(5.0) + 1.) / 2.;
		double c = b - (b - ax) / gr;
		double d = ax + (b - ax) / gr;
		while (fabs(c - d) > tol) {
			if (_R(c, theta, bval, qval) < _R(d, theta, bval, qval)) {
				b = d;
			}
			else {
				ax = c;
			}
			c = b - (b - ax) / gr;
			d = ax + (b - ax) / gr;
		}
		return (ax + b) / 2;
	}

	bool checkRup(double rV, double thetaV, double bV, double qV) {
		if (a_ == 0) return false;
		double min = findMinGoldSec(thetaV, bV, qV, rV, 4 * rV, 0.00001);
		double tmpVal = _R(min, thetaV, bV, qV);
		return (tmpVal >= 0);
	}

	double _b0(double r0) {
		return -(sq3(r0) - 3. * sq(r0) + asq_ * r0 + asq_) / (a_ * (r0 - 1.));
	};

	double _b0diff(double r0) {
		return (asq_ + a_ - 2. * r0 * (sq(r0) - 3. * r0 + 3.)) / (a_ * sq(r0 - 1.));
	};

	double _q0(double r0) {
		return -sq3(r0) * (sq3(r0) - 6. * sq(r0) + 9. * r0 - 4. * asq_) / (asq_ * sq(r0 - 1.));
	};

	/// <summary>
	/// 
	/// </summary>
	/// <param name="bV"></param>
	/// <param name="qV"></param>
	/// <returns>
	/// True if (11) evaluates to false
	/// </returns>
	bool checkB_Q(double bV, double qV) {
		double _r1 = 2. * (1. + cos(2. * acos(-a_) / 3.));
		double _r2 = 2. * (1. + cos(2. * acos(a_) / 3.));
		double error = 0.0000001;
		double r0V = 2.0;
		double bcheck = 100;

		// Newton-Raphson
		while (fabs(bV - bcheck) > error) {
			bcheck = _b0(r0V);
			double bdiffcheck = _b0diff(r0V);
			double rnew = r0V - (bcheck - bV) / bdiffcheck;
			if (rnew < 1) {
				r0V = 1.0001;
			}
			else {
				r0V = rnew;
			}
		}
		double qb = _q0(r0V);
		double _b1 = _b0(_r2);
		double _b2 = _b0(_r1);
		return ((_b1 >= bV) || (_b2 <= bV) || (qV >= qb));
	}

	bool checkCelest(double pRV, double rV, double thetaV, double bV, double qV) {
		bool check1 = checkB_Q(bV, qV);
		bool check2 = !check1 && (pRV < 0);
		bool check4 = checkRup(rV, thetaV, bV, qV);
		bool check3 = check1 && check4;
		return check2 || check3;
	}

	void wrapToPi(double& thetaW, double& phiW) {
		thetaW = fmod(thetaW, PI2);
		while (thetaW < 0) thetaW += PI2;
		if (thetaW > PI) {
			thetaW -= 2 * (thetaW - PI);
			phiW += PI;
		}
		while (phiW < 0) phiW += PI2;
		phiW = fmod(phiW, PI2);
	}

	void wrapToPi(float& thetaW, float& phiW) {
		thetaW = fmod(thetaW, PI2);
		while (thetaW < 0) thetaW += PI2;
		if (thetaW > PI) {
			thetaW -= 2 * (thetaW - PI);
			phiW += PI;
		}
		while (phiW < 0) phiW += PI2;
		phiW = fmod(phiW, PI2);
	}

	void derivs(double* var, double* varOut, double b, double q) {
		double cosv = cos(thetaVar);
		double sinv = sin(thetaVar);
		double cossq = sq(cosv);
		double sinsq = sq(sinv);
		double bsq = sq(b);
		double delta = _Delta(rVar);
		double rosq = _rosq(rVar, thetaVar);
		double P = _P(rVar, b);
		double prsq = sq(pRVar);
		double pthetasq = sq(pThetaVar);
		double R = _R(rVar, thetaVar, b, q);
		double partR = (q + sq(a_ - b));
		double btheta = _BigTheta(rVar, thetaVar, b, q);
		double rosqsq = sq(2 * rosq);
		double sqrosqdel = (sq(rosq) * delta);
		double asqcossin = asq_ * cosv * sinv;
		double rtwo = 2 * rVar - 2;

		// implementation of differential equations
		// dz = affine parameter
		drdz = delta / rosq * pRVar;
		dtdz = 1. / rosq * pThetaVar;

		// see what happens when treating delta*Thau diffrently...
		//dpdz = (2 * A*P - (2 * A - 2 * b)*delta) / (rosq * 2 * delta);
		// result: nothing good...
		dpdz = (2 * a_ * P - (2 * a_ - 2 * b) * delta + (2 * b * cossq * delta) / sinsq) / (rosq * 2 * delta);

		dprdz = (rtwo * btheta - rtwo * partR + 4 * rVar * P) / (rosq * (2 * delta)) - (prsq * rtwo) / (2 * rosq)
			+ (4 * pthetasq * rVar) / rosqsq - ((4 * rVar - 4) * (btheta * (delta)+R)) / (rosq * sq(2 * delta))
			+ (4 * prsq * rVar * (delta)) / rosqsq - (rVar * (btheta * delta + R)) / sqrosqdel;

		dptdz = ((2 * cosv * sinv * (bsq / sinsq - asq_) + (2 * bsq * sq3(cosv)) / sq3(sinv)) * delta) /
			(rosq * 2 * delta) - (4 * asqcossin * pthetasq) / rosqsq - (4 * asqcossin * prsq * delta) /
			rosqsq + (asqcossin * (btheta * delta + R)) / sqrosqdel;
	}

	void rkck(const double* var, const double* dvdz, const int n, const double h,
		double* varOut, double* varErr, const double b, const double q, double* aks,
		double* varTmpInt) {
		int i;
		for (i = 0; i < n; i++)
			varTmpInt[i] = var[i] + b21 * h * dvdz[i];
		derivs(varTmpInt, aks, b, q);
		for (i = 0; i < n; i++)
			varTmpInt[i] = var[i] + h * (b31 * dvdz[i] + b32 * aks[i]);
		derivs(varTmpInt, (aks + 5), b, q);
		for (i = 0; i < n; i++)
			varTmpInt[i] = var[i] + h * (b41 * dvdz[i] + b42 * aks[i] + b43 * aks[i + 5]);
		derivs(varTmpInt, (aks + 10), b, q);
		for (i = 0; i < n; i++)
			varTmpInt[i] = var[i] + h * (b51 * dvdz[i] + b52 * aks[i] + b53 * aks[i + 5] + b54 * aks[i + 10]);
		derivs(varTmpInt, aks + 15, b, q);
		for (i = 0; i < n; i++)
			varTmpInt[i] = var[i] + h * (b61 * dvdz[i] + b62 * aks[i] + b63 * aks[i + 5] + b64 * aks[i + 10] + b65 * aks[i + 15]);
		derivs(varTmpInt, aks + 20, b, q);
		for (i = 0; i < n; i++)
			varOut[i] = var[i] + h * (c1 * dvdz[i] + c3 * aks[i + 5] + c4 * aks[i + 10] + c6 * aks[i + 20]);
		for (i = 0; i < n; i++)
			varErr[i] = h * (dc1 * dvdz[i] + dc3 * aks[i + 5] + dc4 * aks[i + 10] + dc5 * aks[i + 15] + dc6 * aks[i + 20]);
	}

	bool rkqs(double* var, double* dvdz, int n, double& z, double& h,
		const double eps, double* varScal, const double b, const double q,
		double* varErr, double* varTemp, double* aks, double* varTmpInt) {

		rkck(var, dvdz, n, h, varTemp, varErr, b, q, aks, varTmpInt);
		double errmax = 0.0;
		for (int i = 0; i < n; i++) errmax = fmax(errmax, abs(varErr[i] / varScal[i]));
		errmax /= eps;
		if (errmax <= 1.0) {
			z += h;
			for (int i = 0; i < n; i++) var[i] = varTemp[i];
			if (errmax > ERRCON) h = SAFETY * h * pow(errmax, PGROW);
			else h = ADAPTIVE * h;
		}
		else {
			h = fmin(SAFETY * h * pow(errmax, PSHRNK), 0.1 * h);
			return false;
		}
		return true;
	}

	int sgn(float val) {
		return (0 < val) - (val < 0);
	}

	void odeint1(double* varStart, const int nvar, const double zEnd, const double eps,
		const double h1, const double hmin, const double b, const double q, int& step) {
		double varScal[5];
		double var[5];
		double dvdz[5];
		double varErr[5];
		double varTemp[5];
		double aks[25];
		double varTmpInt[5];

		double z = 0.0;
		double h = h1 * sgn(zEnd);

		for (int i = 0; i < nvar; i++) var[i] = varStart[i];

		bool rksuccess = true;
		// should steps that have to be re-taken count to nstp?
		for (int nstp = 0; nstp < MAXSTP; nstp++) {
			// shouldn't this be in an ifelse case, in case rkqs was reset due to large error?
			if (rksuccess) {
				derivs(var, dvdz, b, q);
				for (int i = 0; i < nvar; i++)
					varScal[i] = fabs(var[i]) + fabs(dvdz[i] * h) + TINY;
			}
			else {
				nstp--;
			}

			rksuccess = rkqs(var, dvdz, nvar, z, h, eps, varScal, b, q, varErr, varTemp, aks, varTmpInt);
			step = nstp;
			if (z <= zEnd) {
				for (int i = 0; i < nvar; i++) varStart[i] = var[i];
				return;
			}
		}
	};



	void rkckIntegrate1(const double rV, const double thetaV, const double phiV, const double pRV,
		const double bV, const double qV, const double pThetaV, double& thetaOut, double& phiOut, int& step) {

		double varStart[] = { rV, thetaV, phiV, pRV, pThetaV };
		double to = -10000000;
		double accuracy = 1e-5;
		double stepGuess = 0.01;
		double minStep = 0.00001;

		int n = 5;

		odeint1(varStart, n, to, accuracy, stepGuess, minStep, bV, qV, step);

		thetaOut = varStart[1];
		phiOut = varStart[2];
		wrapToPi(thetaOut, phiOut);

	}


	/// <summary>
	/// Checks if a polygon has a high chance of crossing the 2pi border.
	/// </summary>
	/// <param name="poss">The coordinates of the polygon corners.</param>
	/// <param name="factor">The factor to check whether a point is close to the border.</param>
	/// <returns>Boolean indicating if the polygon is a likely 2pi cross candidate.</returns>
	bool check2PIcross(const std::vector<glm::dvec2>& spl, float factor) {
		for (int i = 0; i < spl.size(); i++) {
			if (spl[i]_phi > PI2 * (1. - 1. / factor))
				return true;
		}
		return false;
	};

	bool check2PIcross(const std::vector<glm::vec2>& spl, float factor) {
		for (int i = 0; i < spl.size(); i++) {
			if (spl[i]_phi > PI2 * (1. - 1. / factor))
				return true;
		}
		return false;
	};

	/// <summary>
	/// Assumes the polygon crosses 2pi and adds 2pi to every corner value of a polygon
	/// that is close (within 2pi/factor) to 0.
	/// </summary>
	/// <param name="poss">The coordinates of the polygon corners.</param>
	/// <param name="factor">The factor to check whether a point is close to the border.</param>
	bool correct2PIcross(std::vector<glm::dvec2>& spl, float factor) {
		bool check = false;
		for (int i = 0; i < spl.size(); i++) {
			if (spl[i]_phi < PI2 * (1. / factor)) {
				spl[i]_phi += PI2;
				check = true;
			}
		}
		return check;
	};

	bool correct2PIcross(std::vector<glm::vec2>& spl, float factor) {
		bool check = false;
		for (int i = 0; i < spl.size(); i++) {
			if (spl[i]_phi < PI2 * (1. / factor)) {
				spl[i]_phi += PI2;
				check = true;
			}
		}
		return check;
	};

	Metric(Metric const&) = delete;
	void operator=(Metric const&) = delete;

private:
	Metric(): a_(0.0), asq_(0.0) {}

	double a_;
	double asq_;

#pragma region constants
	const double
		b21 = 0.2,
		b31 = 3.0 / 40.0, b32 = 9.0 / 40.0,
		b41 = 0.3, b42 = -0.9, b43 = 1.2,
		b51 = -11.0 / 54.0, b52 = 2.5, b53 = -70.0 / 27.0, b54 = 35.0 / 27.0,
		b61 = 1631.0 / 55296.0, b62 = 175.0 / 512.0, b63 = 575.0 / 13824.0, b64 = 44275.0 / 110592.0, b65 = 253.0 / 4096.0;

	const double c1 = 37.0 / 378.0, c3 = 250.0 / 621.0, c4 = 125.0 / 594.0, c6 = 512.0 / 1771.0;

	// dc is difference between c of 5th and 4th order
	const double dc1 = 37.0 / 378.0 - 2825.0 / 27648.0, dc3 = 250.0 / 621.0 - 18575.0 / 48384.0,
		dc4 = 125.0 / 594.0 - 13525.0 / 55296.0, dc5 = -277.00 / 14336.0, dc6 = 512.0 / 1771.0 - 0.25;
#pragma endregion
};