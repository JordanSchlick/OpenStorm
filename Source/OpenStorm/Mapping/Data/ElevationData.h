#pragma once

#include <string>

namespace ElevationData{
	
	void LoadData(std::string dataFilePath);
	
	float GetDataAtPointRadians(double latitude, double longitude);
}