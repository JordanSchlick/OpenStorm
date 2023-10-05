#include <string>
#include <map>


class SimpleConfig{
public:
	std::map<std::string,std::string> values;

	SimpleConfig(std::string fileName);
	bool HasOption(std::string name);
	bool GetBool(std::string name, bool defaultValue = false);
	double GetDouble(std::string name, double defaultValue = 0);
	float GetFloat(std::string name, float defaultValue = 0);
	std::string GetString(std::string name, std::string defaultValue = "");
};