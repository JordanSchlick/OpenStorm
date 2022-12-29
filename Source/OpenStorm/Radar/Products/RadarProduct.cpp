#include "RadarProduct.h"


RadarProduct* RadarProduct::GetProduct(RadarData::VolumeType type){
	if(products.find(type) != products.end()){
		return products[type];
	}else{
		return NULL;
	}
}

int RadarProduct::dynamicVolumeTypeId = 1000;

RadarData::VolumeType RadarProduct::CreateDynamicVolumeType(){
	return (RadarData::VolumeType)dynamicVolumeTypeId++;
}

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