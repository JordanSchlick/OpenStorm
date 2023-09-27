
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
		std::string name = "";
		bool operator < (const FileStats& other) const
		{
			return (name < other.name);
		}
	};
	static double CurrentTime();
	static std::vector<FileStats> ReadDirectory(std::string path);
	static FileStats GetFileStats(std::string path);
	static bool CreateDirectory(std::string path);
	static void Sleep(float seconds);
};