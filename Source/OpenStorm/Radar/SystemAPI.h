
#pragma once

#include <string>
#include <vector>

class SystemAPI{
public:
	static double currentTime();
	static std::vector<std::string> readDirectory(std::string path);
};