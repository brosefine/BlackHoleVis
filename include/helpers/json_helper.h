#pragma once

#include <iostream>
#include <boost/json.hpp>
#include <glm/glm.hpp>

namespace jhelper {

bool getValue(boost::json::object const& obj, std::string const& key, bool& target);
bool getValue(boost::json::object const& obj, std::string const& key, float& target);
bool getValue(boost::json::object const& obj, std::string const& key, int& target);
bool getValue(boost::json::object const& obj, std::string const& key, std::string& target);
bool getValue(boost::json::object const& obj, std::string const& key, glm::vec3& target);
bool getValue(boost::json::object const& obj, std::string const& key, glm::vec2& target);
bool getValue(boost::json::object const& obj, std::string const& key, boost::json::object& target);

} // jhelper
