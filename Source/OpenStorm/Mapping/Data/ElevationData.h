#pragma once

#include <string>

namespace ElevationData{
	// load data into memory from path
	void LoadData(std::string dataFilePath);
	
	// unload data from memory
	void UnloadData();
	
	// get point from the data with interpolation
	float GetDataAtPointRadians(double latitude, double longitude);
}