#pragma once
#include "../RadarData.h"

#include <vector>
#include <map>
#include <string>

// defines a radar product
class RadarProduct {
public:
	enum ProductType{
		// base radar volume products loaded from files
		PRODUCT_BASE,
		// derived radar volume products created from other products
		// use deriveVolume to get the derived volume
		PRODUCT_DERIVED_VOLUME,
		// derived arbatrary data products created from other products
		PRODUCT_DERIVED_DATA,
	};
	
	
	// the type of volume that this product is
	RadarData::VolumeType volumeType = RadarData::VOLUME_UNKNOWN;
	
	// the type of product that this product is
	ProductType productType = PRODUCT_DERIVED_VOLUME;
	
	// if the product is for development and should not be shown by default
	// this could be set because the product is not ready for use or is strictly for debugging
	bool development = false;
	
	// full descriptive name of radar product
	std::string name = "Missing Name";
	
	// short abbreviated name of radar product
	std::string shortName = "MISSING";
	
	// the products that a derived product relies on
	std::vector<RadarData::VolumeType> dependencies = {};
	
	// derive the product from a map containing its dependencies
	// this should not be called if the dependencies are not fulfilled or this is not a derived product
	// inputProducts should contain all dependencies
	// the dependencies may be compressed and need to be be decompressed in deriveVolume
	virtual RadarData* deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts);
	
	// list of all volume products
	static std::vector<RadarProduct*> products;
	//static std::map<RadarData::VolumeType, RadarProduct*> productsMap;
	
	// get the RadarProduct subclass for a given volume type
	static RadarProduct* GetProduct(RadarData::VolumeType type);
	
	// starts at 1000 and is incremented to provide volume types for products defined durring runtime
	static int dynamicVolumeTypeId;
	
	// get new VolumeType for products defined during runtime
	static RadarData::VolumeType CreateDynamicVolumeType();
	
	virtual ~RadarProduct();
};

// class for base radar products
class RadarProductBase : public RadarProduct {
public:
	RadarProductBase(RadarData::VolumeType type, std::string productName, std::string shortProductName);
};