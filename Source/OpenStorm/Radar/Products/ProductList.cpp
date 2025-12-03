#include "RadarProduct.h"
#include "VelocityDealiasProduct.h"
#include "VelocityRelativeProduct.h"
#include "NormalizedRotationProduct.h"
#include "RotationProduct.h"

std::vector<RadarProduct*> RadarProduct::products = {
	new RadarProductBase(RadarData::VOLUME_REFLECTIVITY, "Reflectivity", "REF"),
	new RadarProductBase(RadarData::VOLUME_VELOCITY, "Radial Velocity","RV"),
	new RadarProductBase(RadarData::VOLUME_SPECTRUM_WIDTH, "Spectrum Width","SW"),
	new RadarProductBase(RadarData::VOLUME_CORELATION_COEFFICIENT, "Correlation Coefficient","CC"),
	new RadarProductBase(RadarData::VOLUME_DIFFERENTIAL_REFLECTIVITY, "Differential Reflectivity","DR"),
	new RadarProductBase(RadarData::VOLUME_DIFFERENTIAL_PHASE_SHIFT, "Differential Phase Shift","DPS"),
	new RadarProductVelocityDealiased(),
	new RadarProductStormRelativeVelocity(),
	new RadarProductNormalizedRotation(),
	// new RadarProductRotation(),
	new RadarProductVelocityDealiasedGroupTest(RadarProduct::CreateDynamicVolumeType()),
};

//std::map<RadarData::VolumeType, RadarProduct*> RadarProduct::productsMap = {};

