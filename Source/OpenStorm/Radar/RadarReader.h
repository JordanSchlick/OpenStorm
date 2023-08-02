#pragma once

#include <string>
#include "RadarData.h"


class RadarReader {
public:
	bool verbose = false;
	
	// load in file
	virtual bool LoadFile(std::string filename) = 0;
	
	// load volume into radarData object from loaded file
	virtual bool LoadVolume(RadarData* radarData, RadarData::VolumeType volumeType) = 0;
	
	// unload file and free memory
	virtual void UnloadFile() = 0;
	
	// get a RadarReader object for a given file
	// the type of file will be automatically determined
	// delete the object when finished
	static RadarReader* GetLoaderForFile(std::string filename);
	
	virtual ~RadarReader();
};