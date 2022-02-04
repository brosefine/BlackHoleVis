#pragma once

/* ------------------------------------------------------------------------------------
* Source Code adapted from A.Verbraeck's Blacktracer Black-Hole Visualization
* https://github.com/annemiekie/blacktracer
* https://doi.org/10.1109/TVCG.2020.3030452
* ------------------------------------------------------------------------------------
*/
#include <blacktracer/types.h>
#include <blacktracer/Camera.h>
#include <blacktracer/BlackHole.h>

#include <blacktracer/PSHOffsetTable.h>

#include <vector>
#include <string>
#include <numeric>
#include <algorithm>

#include <unordered_map>
#include <unordered_set>

#include <glm/glm.hpp>
#include <cereal/access.hpp>


class Grid
{
#pragma region cereal
	// Cereal settings for serialization
	friend class cereal::access;
	template < class Archive >
	void serialize(Archive& ar)
	{
		ar(MAXLEVEL, N, M, hasher);
	}

#pragma endregion

#pragma region hashing
	// Hashing functions (2 options)
	struct hashing_func {
		uint64_t operator()(const uint64_t& key) const {
			uint64_t v = key * 3935559000370003845 + 2691343689449507681;

			v ^= v >> 21;
			v ^= v << 37;
			v ^= v >> 4;

			v *= 4768777513237032717;

			v ^= v << 20;
			v ^= v >> 41;
			v ^= v << 5;

			return v;
		}
	};
	struct hashing_func2 {
		uint64_t  operator()(const uint64_t& key) const {
			uint64_t x = key;
			x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
			x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
			x = x ^ (x >> 31);
			return x;
		}
	};

#pragma endregion

public:

	static bool loadFromFile(Grid& outGrid, std::string filename);

	/// <summary>
	/// 1 if rotation axis != camera axis, 0 otherwise
	/// </summary>
	int equafactor;

	/// <summary>
	/// N = max vertical rays, M = max horizontal rays.
	/// </summary>
	int MAXLEVEL, N, M, STARTN, STARTM, STARTLVL;

	bool print = false;

	/// <summary>
	/// Mapping from camera sky position to celestial angle.
	/// </summary>
	std::unordered_map <uint64_t, glm::dvec2, hashing_func2> CamToCel;

	std::vector<int> steps;

	std::unordered_map <uint64_t, glm::dvec2, hashing_func2> CamToAD;

	PSHOffsetTable hasher;

	//PSHOffsetTable hasher;

	/// <summary>
	/// Mapping from block position to level at that point.
	/// </summary>
	std::unordered_map <uint64_t, int, hashing_func2> blockLevels;


	/// <summary>
	/// Initializes an empty new instance of the <see cref="Grid"/> class.
	/// </summary>
	Grid() {};

	/// <summary>
	/// Initializes a new instance of the <see cref="Grid"/> class.
	/// </summary>
	/// <param name="maxLevelPrec">The maximum level for the grid.</param>
	/// <param name="startLevel">The start level for the grid.</param>
	/// <param name="angle">If the camera is not on the symmetry axis.</param>
	/// <param name="camera">The camera.</param>
	/// <param name="bh">The black hole.</param>
	Grid(const int maxLevelPrec, const int startLevel, const bool angle, const Camera* camera, const BlackHole* bh, bool testDisk = false);;


	//void callKernelTEST(const Camera* camera, const BlackHole* bh, size_t s) {
	//	cam = camera;
	//	black = bh;
	//	M = 2048;
	//	N = 512;
	//	equafactor = 0;
	//	std::vector<double> theta(s), phi(s);
	//	int num = 10;
	//	for (int q = 0; q < s; q++) {
	//		theta[q] = (double)(num) / (N - 1) * PI / (2 - equafactor);
	//		phi[q] = (double)(num) / M * PI2;
	//	}
	//	integration_wrapper(theta, phi, s);
	//}

	void saveAsGpuHash();

	/// <summary>
	/// Finalizes an instance of the <see cref="Grid"/> class.
	/// </summary>
	~Grid() {};
private:
	/** ------------------------------ VARIABLES ------------------------------ **/

	bool disk;
	
	// Camera & Blackhole
	const Camera* cam;
	const BlackHole* black;

	// Set of blocks to be checked for division
	std::unordered_set<uint64_t, hashing_func2> checkblocks;


	/** ------------------------------ POST PROCESSING ------------------------------ **/

#pragma region post processing

/// <summary>
/// Returns if a location lies within the boundaries of the provided polygon.
/// </summary>
/// <param name="point">The point (theta, phi) to evaluate.</param>
/// <param name="thphivals">The theta-phi coordinates of the polygon corners.</param>
/// <param name="sgn">The winding order of the polygon (+ for CW, - for CCW).</param>
/// <returns></returns>
	bool pointInPolygon(glm::dvec2& point, std::vector<glm::dvec2>& thphivals, int sgn);

	/// <summary>
	/// Fixes the t-vertices in the grid.
	/// </summary>
	/// <param name="block">The block to check and fix.</param>
	void fixTvertices(std::pair<uint64_t, int> block);

	/// <summary>
	/// Recursively checks the edge of a block for adjacent smaller blocks causing t-vertices.
	/// Adjusts the value of smaller block vertices positioned on the edge to be halfway
	/// inbetween the values at the edges of the larger block.
	/// </summary>
	/// <param name="ij">The key for one of the corners of the block edge.</param>
	/// <param name="ij2">The key for the other corner of the block edge.</param>
	/// <param name="level">The level of the block.</param>
	/// <param name="udlr">1=down, 0=right</param>
	/// <param name="lr">1=right</param>
	/// <param name="gap">The gap at the current level.</param>
	void checkAdjacentBlock(uint64_t ij, uint64_t ij2, int level, int udlr, int gap);

	bool find(uint64_t ij) {
		return CamToCel.find(ij) != CamToCel.end();
	}

	glm::dvec2 const hermite(double aValue, glm::dvec2 const& aX0, glm::dvec2 const& aX1, glm::dvec2 const& aX2, glm::dvec2 const& aX3, double aTension, double aBias);




#pragma endregion

	/** -------------------------------- RAY TRACING -------------------------------- **/

	/// <summary>
	/// Prints the grid cam.
	/// </summary>
	/// <param name="level">The level.</param>
	void printGridCam(int level);

	/// <summary>
	/// Raytraces this instance.
	/// </summary>
	void raytrace();

	/// <summary>
	/// Integrates the first blocks.
	/// </summary>
	/// <param name="gap">The gap at the current trace level.</param>
	void integrateFirst(const int gap);

	/// <summary>
	/// Fills the grid map with the just computed raytraced values.
	/// </summary>
	/// <param name="ijvals">The original keys for which rays where traced.</param>
	/// <param name="s">The size of the vectors.</param>
	/// <param name="thetavals">The computed theta values (celestial sky).</param>
	/// <param name="phivals">The computed phi values (celestial sky).</param>
	void fillGridCam(const std::vector<uint64_t>& ijvals, const size_t s, std::vector<double>& thetavals,
		std::vector<double>& phivals, std::vector<double>& hitr, std::vector<double>& hitphi, std::vector<int>& step);

	template <typename T, typename Compare>
	std::vector<std::size_t> sort_permutation(
		const std::vector<T>& vec, const std::vector<T>& vec1,
		Compare& compare) {
		std::vector<std::size_t> p(vec.size());
		std::iota(p.begin(), p.end(), 0);
		std::sort(p.begin(), p.end(),
			[&](std::size_t i, std::size_t j) { return compare(vec[i], vec[j], vec1[i], vec1[j]); });
		return p;
	}

	template <typename T>
	std::vector<T> apply_permutation(
		const std::vector<T>& vec,
		const std::vector<std::size_t>& p) {
		std::vector<T> sorted_vec(vec.size());
		std::transform(p.begin(), p.end(), sorted_vec.begin(),
			[&](std::size_t i) { return vec[i]; });
		return sorted_vec;
	}


	/// <summary>
	/// Calls the ~kernel~ the raytracing code.
	/// </summary>
	/// <param name="ijvec">The ijvec.</param>
	void callKernel(std::vector<uint64_t>& ijvec);

	/// <summary>
	/// Returns if a block needs to be refined.
	/// </summary>
	/// <param name="i">The i position of the block.</param>
	/// <param name="j">The j position of the block.</param>
	/// <param name="gap">The current block gap.</param>
	/// <param name="level">The current block level.</param>
	bool refineCheck(const uint32_t i, const uint32_t j, const int gap, const int level);;

	/// <summary>
	/// Fills the toIntIJ vector with unique instances of theta-phi combinations.
	/// </summary>
	/// <param name="toIntIJ">The vector to store the positions in.</param>
	/// <param name="i">The i key - to be translated to theta.</param>
	/// <param name="j">The j key - to be translated to phi.</param>
	void fillVector(std::vector<uint64_t>& toIntIJ, uint32_t i, uint32_t j);

	/// <summary>
	/// Adaptively raytraces the grid.
	/// </summary>
	/// <param name="level">The current level.</param>
	void adaptiveBlockIntegration(int level);

	/// <summary>
	/// Raytraces the rays starting in camera sky from the theta, phi positions defined
	/// in the provided vectors.
	/// </summary>
	/// <param name="theta">The theta positions.</param>
	/// <param name="phi">The phi positions.</param>
	/// <param name="n">The size of the vectors.</param>
	void integration_wrapper(std::vector<double>& theta, std::vector<double>& phi, const int n, std::vector<int>& step);

};
