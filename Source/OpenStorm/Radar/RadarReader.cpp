#include "RadarReader.h"
#include "Nexrad.h"
#include "OdimH5.h"
#include "MiniRad.h"
#include <algorithm>
#include <cctype>

inline bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

RadarReader* RadarReader::GetLoaderForFile(const std::string filename){
	std::string filenameLower = filename + "";
	std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), [](unsigned char c){ return std::tolower(c); });
	if(hasEnding(filenameLower, ".h5") || hasEnding(filenameLower, ".hdf5") || hasEnding(filenameLower, ".hdf")){
		return new OdimH5RadarReader();
	}
	if(hasEnding(filenameLower, ".gz") || hasEnding(filenameLower, ".bz2")){
		return new NexradRadarReader();
	}
	if(hasEnding(filenameLower, ".minirad")){
		return new MiniRadRadarReader();
	}
	// TODO: read a small section of the file if the type is not apparent from the name and then analyze it
	return new NexradRadarReader();
}

RadarReader::~RadarReader(){
	
}
