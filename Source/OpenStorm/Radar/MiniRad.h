#pragma once

#include "RadarReader.h"

class MiniRadRadarReader : public RadarReader{
public:
	class MiniRadInternal;
	MiniRadInternal* internal = NULL;
	
	virtual bool LoadFile(std::string filename);
	
	virtual bool LoadVolume(RadarData* radarData, RadarData::VolumeType volumeType);
	
	virtual void UnloadFile();
	
	virtual ~MiniRadRadarReader();
};