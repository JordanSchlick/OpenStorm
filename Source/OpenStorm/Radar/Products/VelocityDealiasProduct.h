#include "RadarProduct.h"

class RadarProductVelocityDealiased : public RadarProduct {
public:
	static bool verbose;
	RadarProductVelocityDealiased();
	virtual RadarData* deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts) override;
};

class RadarProductVelocityDealiasedGroupTest : public RadarProduct {
public:
	RadarProductVelocityDealiasedGroupTest(RadarData::VolumeType dynamicVolumeType);
	virtual RadarData* deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts) override;
};
