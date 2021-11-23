#pragma once

#include <chrono>
#include <vector>
#include <string>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

class Timer {
public:
	void start();
	void start(std::string sec);
	void end();

	void printLast() const;
	double getLast() const;
	void print(int index) const;
	void summary() const;

private:
	TimePoint start_;
	bool active_section_ = false;

	std::vector<std::chrono::duration<double>> durations_;
	std::vector<std::string> sections_;

	int section_count_ = 1;
};
