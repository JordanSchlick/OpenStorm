#include <vector>
#include <string>
#include "../../Radar/SimpleVector3.h"

class GISObject {
public:
	// center in lat lon
	// SimpleVector3<float> center;
	
	// location in cartesian space on an un-oriented globe in meters
	SimpleVector3<float> location;
	// if the shape is made up of many points or just one
	bool isGeometry = false;
	// array pairs of latitude,longitude floats and separate parts are delineated by NaNs
	float* geometry = NULL;
	// number of floats in geometry buffer
	uint32_t geometryCount = 0;
	// if the object is currently show in game
	bool shown = false;
	
	void Delete();
};

bool ReadShapeFile(std::string fileName, std::vector<GISObject>* output);