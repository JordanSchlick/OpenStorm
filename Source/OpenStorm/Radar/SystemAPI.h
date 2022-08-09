
#pragma once

#include <string>
#include <vector>

class SystemAPI{
public:
	static double CurrentTime();
	static std::vector<std::string> ReadDirectory(std::string path);
};