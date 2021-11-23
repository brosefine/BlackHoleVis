#include<helpers/Timer.hpp>
#include <stdexcept>
#include <string>
#include <iostream>

using namespace std::chrono;

void Timer::start() {
	start("Section " + std::to_string(section_count_++));
}

void Timer::start(std::string sec) {
	if (active_section_) throw std::invalid_argument("Last section as not been ended.");
	active_section_ = true;
	sections_.push_back(sec);
	start_ = Clock::now();
}

void Timer::end() {
	// stop time before we do anything else
	auto end = Clock::now();
	if (!active_section_) throw std::invalid_argument("No active section.");
	active_section_ = false;
	durations_.push_back(end-start_);
}

void Timer::printLast() const {
	if (active_section_) throw std::invalid_argument("Can't print in an active section.");
	std::cout << sections_.back() << ": " 
			<< durations_.back().count() << "s" 
			<< std::endl;
}

double Timer::getLast() const {
	if (active_section_) throw std::invalid_argument("Can't return when in an active section.");
	return durations_.back().count();
}

void Timer::print(int index) const {
	if (active_section_) throw std::invalid_argument("Can't print in an active section.");
	std::cout << sections_.at(index) << ": " 
			<< durations_.at(index).count() << "s" 
			<< std::endl;
}

void Timer::summary() const {
	if (active_section_) throw std::invalid_argument("Can't print in an active section.");

	duration<double> sum;
	for(auto const& d : durations_) {
		sum += d;
	}

	std::cout << "\n ####### Summary ####### \n" << std::endl;
	auto s_duration = sum.count();
	std::cout << "Total duration: " 
			<< s_duration << "s" << std::endl;

	for(int i = 0; i < durations_.size(); ++i) {
		auto d = durations_.at(i).count();
		std::cout << sections_.at(i) << ": " 
				<< d << "s = " 
				<< d*100/s_duration << "%"
				<< std::endl;

	}
}