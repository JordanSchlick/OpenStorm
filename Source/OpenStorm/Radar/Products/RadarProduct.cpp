#include "RadarProduct.h"

RadarData *RadarProduct::deriveVolume(std::map<RadarData::VolumeType, RadarData *> inputProducts) {
	return NULL;
}

RadarProduct::~RadarProduct(){
	
}

RadarProductBase::RadarProductBase(RadarData::VolumeType type, std::string productName, std::string shortProductName){
	volumeType = type;
	productType = PRODUCT_BASE;
	name = productName;
	shortName = shortProductName;
}