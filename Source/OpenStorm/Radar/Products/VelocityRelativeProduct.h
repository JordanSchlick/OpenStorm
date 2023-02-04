#include "RadarProduct.h"

class RadarProductStormRelativeVelocity : public RadarProduct {
public:
	RadarProductStormRelativeVelocity();
	virtual RadarData* deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts) override;
};