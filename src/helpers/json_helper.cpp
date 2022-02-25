#include <helpers/json_helper.h>

namespace jhelper {

	bool getValue(boost::json::object const& obj, std::string const& key, bool& target) {
		if (!obj.contains(key) || obj.at(key).kind() != boost::json::kind::bool_) {
			std::cerr << "[JSON] key " << key << " not read" << std::endl;
			return false;
		}

		target = obj.at(key).get_bool();
		return true;
	}

	bool getValue(boost::json::object const& obj, std::string const& key, float& target) {
		if (!obj.contains(key) || obj.at(key).kind() != boost::json::kind::double_) {
			std::cerr << "[JSON] key " << key << " not read" << std::endl;
			return false;
		}

		target = obj.at(key).get_double();
		return true;
	}

	bool getValue(boost::json::object const& obj, std::string const& key, int& target) {
		if (!obj.contains(key) || obj.at(key).kind() != boost::json::kind::int64) {
			std::cerr << "[JSON] key " << key << " not read" << std::endl;
			return false;
		}

		target = obj.at(key).get_int64();
		return true;
	}

	bool getValue(boost::json::object const& obj, std::string const& key, std::string& target) {
		if (!obj.contains(key) || obj.at(key).kind() != boost::json::kind::string) {
			std::cerr << "[JSON] key " << key << " not read" << std::endl;
			return false;
		}

		target = obj.at(key).get_string();
		return true;
	}

	bool getValue(boost::json::object const& obj, std::string const& key, glm::vec3& target) {
		if (!obj.contains(key) || obj.at(key).kind() != boost::json::kind::array) {
			std::cerr << "[JSON] key " << key << " not read" << std::endl;
			return false;
		}

		auto values = obj.at(key).get_array();
		target.x = values[0].get_double();
		target.y = values[1].get_double();
		target.z = values[2].get_double();
		return true;
	}

	bool getValue(boost::json::object const& obj, std::string const& key, glm::vec2& target) {
		if (!obj.contains(key) || obj.at(key).kind() != boost::json::kind::array) {
			std::cerr << "[JSON] key " << key << " not read" << std::endl;
			return false;
		}

		auto values = obj.at(key).get_array();
		target.x = values[0].get_double();
		target.y = values[1].get_double();
		return true;
	}

	bool getValue(boost::json::object const& obj, std::string const& key, boost::json::object& target) {
		if (!obj.contains(key) || obj.at(key).kind() != boost::json::kind::object) {
			std::cerr << "[JSON] key " << key << " not read" << std::endl;
			return false;
		}

		target = obj.at(key).get_object();
		return true;
	}

	bool getValue(boost::json::object const& obj, std::string const& key, double& target)	{
		if (!obj.contains(key) || obj.at(key).kind() != boost::json::kind::double_) {
			std::cerr << "[JSON] key " << key << " not read" << std::endl;
			return false;
		}

		target = obj.at(key).get_double();
		return true;
	}

} // jhelper
