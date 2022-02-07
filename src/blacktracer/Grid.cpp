#include <blacktracer/Grid.h>

/* ------------------------------------------------------------------------------------
* Source Code adapted from A.Verbraeck's Blacktracer Black-Hole Visualization
* https://github.com/annemiekie/blacktracer
* https://doi.org/10.1109/TVCG.2020.3030452
* ------------------------------------------------------------------------------------
*/

#include <blacktracer/Metric.h>
#include <blacktracer/Code.h>
#include <helpers/RootDir.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <format>

#include <cereal/archives/binary.hpp>


#define PRECCELEST 0.015
#define ERROR 0.001//1e-6


bool Grid::makeGrid(std::shared_ptr<Grid>& outGrid, GridProperties props) {
	if (loadFromFile(outGrid, props)) {
		std::cout << "[GRID] loaded grid from file." << std::endl;
		return true;
	}

	outGrid = std::make_shared<Grid>(props);
	std::cout << "[GRID] generated new grid." << std::endl;
	return false;
}

bool Grid::loadFromFile(std::shared_ptr<Grid>& outGrid, std::string filename)
{
	std::ifstream ifs(ROOT_DIR "resources/grids/" + filename, std::ios::in | std::ios::binary);
	if (!ifs.good()) {
		std::cerr << "[GRID] couldn't load grid file" << std::endl;
		return false;
	}
	cereal::BinaryInputArchive iarch(ifs);
	iarch(*outGrid);
	return true;
}

bool Grid::loadFromFile(std::shared_ptr<Grid>& outGrid, GridProperties props)
{
	return Grid::loadFromFile(outGrid, getFileNameFromConfig(props));
}

bool Grid::saveToFile(std::shared_ptr<Grid> inGrid) {

	std::string filename = ROOT_DIR "resources/grids/" + inGrid->getFileNameFromConfig();
	std::ofstream ofs(filename, std::ios::out | std::ios::binary);
	cereal::BinaryOutputArchive oarch(ofs);
	oarch(*inGrid);
	return true;
}

std::string Grid::getFileNameFromConfig(GridProperties const& props) {
	return std::format(
		"rayTraceLvl-strt-{}-max-{}_pos-r-{:.2f}-the-{:.2f}-phi-{:.2f}_vel-{:.2f}_spin-{}.grid",
		props.grid_strtLvl_, props.grid_maxLvl_,
		props.cam_rad_, props.cam_the_, props.cam_phi_, props.cam_vel_,
		props.blackHole_a_
	);
}

std::string Grid::getFileNameFromConfig() const {
	return getFileNameFromConfig(props_);
}

Grid::Grid(const int maxLevelPrec, const int startLevel, const bool angle, std::shared_ptr<Camera> camera, std::shared_ptr<BlackHole> bh, bool testDisk /*= false*/)
{
	disk = testDisk;
	MAXLEVEL = maxLevelPrec;
	STARTLVL = startLevel;
	cam = camera;
	black = bh;
	equafactor = angle ? 1 : 0;

	init();
	
};

Grid::Grid(GridProperties props)
	: equafactor(1)	// ignore symmetry for now
	, MAXLEVEL(props.grid_maxLvl_)
	, STARTLVL(props.grid_strtLvl_)
	, disk(false) // ... and disk as well
{
	cam = std::make_shared<Camera>(
		props.cam_the_, props.cam_phi_,
		props.cam_rad_, props.cam_vel_
	);

	black = std::make_shared<BlackHole>(props.blackHole_a_);

	init();
};

void Grid::init() {
	N = (uint32_t)round(pow(2, MAXLEVEL) / (2 - equafactor) + 1);
	STARTN = (uint32_t)round(pow(2, STARTLVL) / (2 - equafactor) + 1);
	M = (2 - equafactor) * 2 * (N - 1);
	STARTM = (2 - equafactor) * 2 * (STARTN - 1);
	steps = std::vector<int>(M * N);
	raytrace();
	//printGridCam(5);

	for (auto block : blockLevels) {
		fixTvertices(block);
	}
	if (STARTLVL != MAXLEVEL) saveAsGpuHash();
}

void Grid::saveAsGpuHash()
{
	if (hasher.n > 0) return;

	if (print) std::cout << "Computing Perfect Hash.." << std::endl;

	std::vector<glm::ivec2> elements;
	std::vector<glm::vec2> data;
	for (auto entry : CamToCel) {
		//FIX: conversion from uint64 to int32 is narrowing conversion - cast to int
		elements.push_back({ (int)(entry.first >> 32), (int)(entry.first) });
		//FIX: conversion from double to float is narrowing conversion - explicitly cast to float
		data.push_back({ (float)entry.second.x, (float)entry.second.y });
	}
	hasher = PSHOffsetTable(elements, data);

	if (print) std::cout << "Completed Perfect Hash" << std::endl;
}

bool Grid::pointInPolygon(glm::dvec2& point, std::vector<glm::dvec2>& thphivals, int sgn)
{
	for (int q = 0; q < 4; q++) {
		glm::dvec2 vecLine = (double)sgn * (thphivals[q] - thphivals[(q + 1) % 4]);
		glm::dvec2 vecPoint = sgn ? (point - thphivals[(q + 1) % 4]) : (point - thphivals[q]);
		// cross product < 0?
		if ((vecLine.x*vecPoint.y - vecLine.y*vecPoint.x) < 0) {
			return false;
		}
	}
	return true;
}

void Grid::fixTvertices(std::pair<uint64_t, int> block)
{
	int level = block.second;
	if (level == MAXLEVEL) return;
	uint64_t ij = block.first;
	if (CamToCel[ij]_phi < 0) return;

	int gap = pow(2, MAXLEVEL - level);
	uint32_t i = i_32;
	uint32_t j = j_32;
	uint32_t k = i + gap;
	uint32_t l = (j + gap) % M;

	checkAdjacentBlock(ij, k_j, level, 1, gap);
	checkAdjacentBlock(ij, i_l, level, 0, gap);
	checkAdjacentBlock(i_l, k_l, level, 1, gap);
	checkAdjacentBlock(k_j, k_l, level, 0, gap);
}

void Grid::checkAdjacentBlock(uint64_t ij, uint64_t ij2, int level, int udlr, int gap)
{
	uint32_t i = i_32 + udlr * gap / 2;
	uint32_t j = j_32 + (1 - udlr) * gap / 2;
	auto it = CamToCel.find(i_j);
	if (it == CamToCel.end())
		return;
	else {
		uint32_t jprev = (j_32 - (1 - udlr) * gap + M) % M;
		uint32_t jnext = (j_32 + (1 - udlr) * 2 * gap) % M;
		uint32_t iprev = i_32 - udlr * gap;
		uint32_t inext = i_32 + 2 * udlr * gap;

		bool half = false;

		if (i_32 == 0) {
			jprev = (j_32 + M / 2) % M;
			iprev = gap;
		}
		else if (inext > N - 1) {
			inext = i_32;
			if (equafactor) jnext = (j_32 + M / 2) % M;
			else half = true;
		}
		uint64_t ijprev = (uint64_t)iprev << 32 | jprev;
		uint64_t ijnext = (uint64_t)inext << 32 | jnext;

		bool succes = false;
		if (find(ijprev) && find(ijnext)) {
			std::vector<glm::dvec2> check = { CamToCel[ijprev], CamToCel[ij], CamToCel[ij2], CamToCel[ijnext] };
			if (CamToCel[ijprev] != glm::dvec2(-1, -1) && CamToCel[ijnext] != glm::dvec2(-1, -1)) {
				succes = true;
				if (half) check[3].x = PI - check[3].x;
				if (metric::check2PIcross(check, 5.)) metric::correct2PIcross(check, 5.);
				CamToCel[i_j] = hermite(0.5, check[0], check[1], check[2], check[3], 0., 0.);
			}
		}
		if (!succes) {
			std::vector<glm::dvec2> check = { CamToCel[ij], CamToCel[ij2] };
			if (metric::check2PIcross(check, 5.)) metric::correct2PIcross(check, 5.);
			CamToCel[i_j] = 1. / 2. * (check[1] + check[0]);
		}
		if (level + 1 == MAXLEVEL) return;
		checkAdjacentBlock(ij, i_j, level + 1, udlr, gap / 2);
		checkAdjacentBlock(i_j, ij2, level + 1, udlr, gap / 2);
	}
}

glm::dvec2 const Grid::hermite(double aValue, glm::dvec2 const& aX0, glm::dvec2 const& aX1, glm::dvec2 const& aX2, glm::dvec2 const& aX3, double aTension, double aBias)
{
	/* Source:
	* http://paulbourke.net/miscellaneous/interpolation/
	*/

	double const v = aValue;
	double const v2 = v * v;
	double const v3 = v * v2;

	double const aa = (double(1) + aBias) * (double(1) - aTension) / double(2);
	double const bb = (double(1) - aBias) * (double(1) - aTension) / double(2);

	glm::dvec2 const m0 = aa * (aX1 - aX0) + bb * (aX2 - aX1);
	glm::dvec2 const m1 = aa * (aX2 - aX1) + bb * (aX3 - aX2);

	double const u0 = double(2) * v3 - double(3) * v2 + double(1);
	double const u1 = v3 - double(2) * v2 + v;
	double const u2 = v3 - v2;
	double const u3 = double(-2) * v3 + double(3) * v2;

	return u0 * aX1 + u1 * m0 + u2 * m1 + u3 * aX2;
}

void Grid::printGridCam(int level)
{
	if (level > MAXLEVEL) {
		std::cerr << "[Grid]: invalid level at printGridCam" << std::endl;
		return;
	}
	
	if (CamToCel.size() <= 0) {
		std::cerr << "[Grid]: cannot print empty Grid data (probably because it was loaded from file)" << std::endl;
		return;
	}

	std::cout.precision(2);
	std::cout << std::endl;

	int gap = (int)pow(2, MAXLEVEL - level);
	for (uint32_t i = 0; i < N; i += gap) {
		for (uint32_t j = 0; j < M; j += gap) {
			double val = CamToCel[i_j]_theta;
			if (val > 1e-10)
				std::cout << std::setw(4) << val / PI;
			else
				std::cout << std::setw(4) << 0.0;
		}
		std::cout << std::endl;
	}

	std::cout << std::endl;
	for (uint32_t i = 0; i < N; i += gap) {
		for (uint32_t j = 0; j < M; j += gap) {
			double val = CamToCel[i_j]_phi;
			if (val > 1e-10)
				std::cout << std::setw(4) << val / PI;
			else
				std::cout << std::setw(4) << 0.0;
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

	std::cout << std::endl;
	for (uint32_t i = 0; i < N; i += gap) {
		for (uint32_t j = 0; j < M; j += gap) {
			double val = steps[i * M + j];
			std::cout << std::setw(4) << val;
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

	int sum = 0;
	int sumnotnull = 0;
	int countnotnull = 0;
	std::ofstream myfile;
	myfile.open("steps.txt");
	for (int i = 0; i < N * M; i++) {
		sum += steps[i];
		if (steps[i] > 0) {
			sumnotnull += steps[i];
			countnotnull++;
		}
		myfile << steps[i] << "\n";
	}
	myfile.close();
	std::cout << "steeeeps" << sum << std::endl;
	std::cout << "steeeepsnotnull" << sumnotnull << std::endl;

	std::cout << "ave" << (float)sum / (float)(M * (N + 1)) << std::endl;
	std::cout << "avenotnull" << (float)sumnotnull / (float)(countnotnull) << std::endl;

	//for (uint32_t i = 0; i < N; i += gap) {
	//	for (uint32_t j = 0; j < M; j += gap) {
	//		int val = CamToAD[i_j];
	//		std::cout << std::setw(4) << val;
	//	}
	//	std::cout << std::endl;
	//}
	//std::cout << std::endl;

	std::cout.precision(10);
}

void Grid::raytrace()
{
	int gap = (int)pow(2, MAXLEVEL - STARTLVL);
	int s = (1 + equafactor);

	std::vector<uint64_t> ijstart(s);

	ijstart[0] = 0;
	if (equafactor) ijstart[1] = (uint64_t)(N - 1) << 32;

	if (print) std::cout << "Computing Level " << STARTLVL << "..." << std::endl;
	callKernel(ijstart);

	for (uint32_t j = 0; j < M; j += gap) {
		uint32_t i, l, k;
		i = l = k = 0;
		CamToCel[i_j] = CamToCel[k_l];
		steps[i * M + j] = steps[0];
		checkblocks.insert(i_j);
		if (equafactor) {
			i = k = N - 1;
			CamToCel[i_j] = CamToCel[k_l];
			steps[i * M + j] = steps[0];

		}
	}

	integrateFirst(gap);
	adaptiveBlockIntegration(STARTLVL);
}

void Grid::integrateFirst(const int gap)
{
	std::vector<uint64_t> toIntIJ;

	for (uint32_t i = gap; i < N - equafactor; i += gap) {
		for (uint32_t j = 0; j < M; j += gap) {
			toIntIJ.push_back(i_j);
			if (i == N - 1);// && !equafactor);
			else if (MAXLEVEL == STARTLVL) blockLevels[i_j] = STARTLVL;
			else checkblocks.insert(i_j);
		}
	}
	callKernel(toIntIJ);

}

void Grid::fillGridCam(const std::vector<uint64_t>& ijvals, const size_t s, std::vector<double>& thetavals, std::vector<double>& phivals, std::vector<double>& hitr, std::vector<double>& hitphi, std::vector<int>& step)
{
	for (int k = 0; k < s; k++) {
		CamToCel[ijvals[k]] = glm::dvec2(thetavals[k], phivals[k]);
		uint64_t ij = ijvals[k];
		steps[i_32 * M + j_32] = step[k];
		//if (disk) CamToAD[ijvals[k]] = glm::dvec2(hitr[k], hitphi[k]);
	}
}

void Grid::callKernel(std::vector<uint64_t>& ijvec)
{
	size_t s = ijvec.size();
	std::vector<double> theta(s), phi(s);
	std::vector<int> step(s);
	for (int q = 0; q < s; q++) {
		uint64_t ij = ijvec[q];
		theta[q] = (double)i_32 / (N - 1) * PI / (2 - equafactor);
		phi[q] = (double)j_32 / M * PI2;
	}

	auto start_time = std::chrono::high_resolution_clock::now();
	integration_wrapper(theta, phi, s, step);
	std::vector<double> e1, e2;
	fillGridCam(ijvec, s, theta, phi, e1, e2, step);
	auto end_time = std::chrono::high_resolution_clock::now();
	int count = 0;
	for (int q = 0; q < s; q++) if (step[q] != 0) count++;
	std::cout << "CPU: " << count << "rays in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms!" << std::endl << std::endl;
	//}
}

bool Grid::refineCheck(const uint32_t i, const uint32_t j, const int gap, const int level)
{
	uint32_t k = i + gap;
	uint32_t l = (j + gap) % M;
	/*
	if (disk) {
		double a = CamToAD[i_j].x;
		double b = CamToAD[k_j].x;
		double c = CamToAD[i_l].x;
		double d = CamToAD[k_l].x;
		if (a > 0 || b > 0 || c > 0 || d > 0) {
			return true;
		}
	}
	*/

	double th1 = CamToCel[i_j]_theta;
	double th2 = CamToCel[k_j]_theta;
	double th3 = CamToCel[i_l]_theta;
	double th4 = CamToCel[k_l]_theta;

	double ph1 = CamToCel[i_j]_phi;
	double ph2 = CamToCel[k_j]_phi;
	double ph3 = CamToCel[i_l]_phi;
	double ph4 = CamToCel[k_l]_phi;

	double diag = (th1 - th4) * (th1 - th4) + (ph1 - ph4) * (ph1 - ph4);
	double diag2 = (th2 - th3) * (th2 - th3) + (ph2 - ph3) * (ph2 - ph3);

	double max = std::max(diag, diag2);

	if (level < 6 && max>1E-10) return true;
	if (max > PRECCELEST) return true;

	// If no refinement necessary, save level at position.
	blockLevels[i_j] = level;
	return false;

};

void Grid::fillVector(std::vector<uint64_t>& toIntIJ, uint32_t i, uint32_t j)
{
	auto iter = CamToCel.find(i_j);
	if (iter == CamToCel.end()) {
		toIntIJ.push_back(i_j);
		CamToCel[i_j] = glm::dvec2(-10, -10);
	}
}

void Grid::adaptiveBlockIntegration(int level)
{
	while (level < MAXLEVEL) {
		if (level < 5 && print) printGridCam(level);
		if (print) std::cout << "Computing level " << level + 1 << "..." << std::endl;

		if (checkblocks.size() == 0) return;

		std::unordered_set<uint64_t, hashing_func2> todo;
		std::vector<uint64_t> toIntIJ;

		for (auto ij : checkblocks) {

			uint32_t gap = (uint32_t)pow(2, MAXLEVEL - level);
			uint32_t i = i_32;
			uint32_t j = j_32;
			uint32_t k = i + gap / 2;
			uint32_t l = j + gap / 2;
			j = j % M;

			if (refineCheck(i, j, gap, level)) {
				fillVector(toIntIJ, k, j);
				fillVector(toIntIJ, k, l);
				fillVector(toIntIJ, i, l);
				fillVector(toIntIJ, i + gap, l);
				fillVector(toIntIJ, k, (j + gap) % M);
				todo.insert(i_j);
				todo.insert(k_j);
				todo.insert(k_l);
				todo.insert(i_l);
			}

		}
		callKernel(toIntIJ);
		level++;
		checkblocks = todo;
	}

	for (auto ij : checkblocks)
		blockLevels[ij] = level;
}

void Grid::integration_wrapper(std::vector<double>& theta, std::vector<double>& phi, const int n, std::vector<int>& step)
{
	double thetaS = cam->theta;
	double phiS = cam->phi;
	double rS = cam->r;
	double sp = cam->speed;

#pragma loop(hint_parallel(8))
#pragma loop(ivdep)
	for (int i = 0; i < n; i++) {

		double xCam = sin(theta[i]) * cos(phi[i]);
		double yCam = sin(theta[i]) * sin(phi[i]);
		double zCam = cos(theta[i]);

		double yFido = (-yCam + sp) / (1 - sp * yCam);
		double xFido = -sqrtf(1 - sp * sp) * xCam / (1 - sp * yCam);
		double zFido = -sqrtf(1 - sp * sp) * zCam / (1 - sp * yCam);

		double k = sqrt(1 - cam->btheta * cam->btheta);
		double rFido = xFido * cam->bphi / k + cam->br * yFido + cam->br * cam->btheta / k * zFido;
		double thetaFido = cam->btheta * yFido - k * zFido;
		double phiFido = -xFido * cam->br / k + cam->bphi * yFido + cam->bphi * cam->btheta / k * zFido;
		//double rFido = xFido;
		//double thetaFido = -zFido;
		//double phiFido = yFido;

		double eF = 1. / (cam->alpha + cam->w * cam->wbar * phiFido);

		double pR = eF * cam->ro * rFido / sqrtf(cam->Delta);
		double pTheta = eF * cam->ro * thetaFido;
		double pPhi = eF * cam->wbar * phiFido;

		double b = pPhi;
		double q = pTheta * pTheta + cos(thetaS) * cos(thetaS) * (b * b / (sin(thetaS) * sin(thetaS)) - *metric::asq);

		theta[i] = -1;
		phi[i] = -1;
		step[i] = 0;

		if (metric::checkCelest(pR, rS, thetaS, b, q)) {
			metric::rkckIntegrate1(rS, thetaS, phiS, pR, b, q, pTheta, theta[i], phi[i], step[i]);
		}
	}
}

