#pragma once

#include <imgui.h>

#include <string>

namespace imgui_helpers {

	// get imgui input as float and store in var
	bool sliderDouble(std::string label, double& var, double min, double max) {
		float tmpVar = var;
		if (ImGui::SliderFloat(label.c_str(), &tmpVar, (float)min, (float)max)) {
			var = tmpVar;
			return true;
		}
		return false;
	}
}