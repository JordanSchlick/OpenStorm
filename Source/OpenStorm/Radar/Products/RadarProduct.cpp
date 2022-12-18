#include "RadarProduct.h"

RadarData *RadarProduct::deriveVolume(std::map<RadarData::VolumeType, RadarData *> inputProducts) {
	return NULL;
}

RadarProductBase::RadarProductBase(RadarData::VolumeType type, std::string productName){
	volumeType = type;
	productType = PRODUCT_BASE;
	name = productName;
}