#pragma once

#include "RadarReader.h"

namespace Nexrad{
	// recompress an archive using gzip, retuns zero on success
	int RecompressArchive(std::string inFile, std::string outFile);
	// decompress an archive, returns zero on success
	int DecompressArchive(std::string inFile, std::string outFile);
};


class NexradRadarReader : public RadarReader{
public:
	void* rslData = NULL;
	
	virtual bool LoadFile(std::string filename);
	
	virtual bool LoadVolume(RadarData* radarData, RadarData::VolumeType volumeType);
	
	virtual void UnloadFile();
	
	virtual ~NexradRadarReader();
};