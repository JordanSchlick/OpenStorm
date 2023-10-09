#pragma once

#include "RadarReader.h"

class OdimH5RadarReader : public RadarReader{
public:
	class OdimH5Internal;
	class SweepData;
	OdimH5Internal* internal = NULL;
	
	virtual bool LoadFile(std::string filename);
	
	virtual bool LoadVolume(RadarData* radarData, RadarData::VolumeType volumeType);
	
	virtual void UnloadFile();
	
	virtual ~OdimH5RadarReader();
};