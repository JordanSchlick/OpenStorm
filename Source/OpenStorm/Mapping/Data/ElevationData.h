#pragma once

#include <string>

namespace ElevationData{
	// load data into memory from path
	void LoadData(std::string dataFilePath);
	
	// unload data from memory
	void UnloadData();
	
	// get point from the data with interpolation
	float GetDataAtPointRadians(double latitude, double longitude);
	
	// load and increment reference counter
	// make sure to call StopUsing after
	void StartUsing();
	
	// decrement reference counter and unload
	// make sure to call StartUsing before
	void StopUsing();
}