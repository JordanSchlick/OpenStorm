
#pragma once

#include <string>
#include <vector>

class SystemAPI{
public:
	struct FileStats{
		bool exists = false;
		bool isDirectory = false;
		size_t size = 0;	
		double mtime = 0;	
	};
	static double CurrentTime();
	static std::vector<std::string> ReadDirectory(std::string path);
	static FileStats GetFileStats(std::string path);
	static bool CreateDirectory(std::string path);
};