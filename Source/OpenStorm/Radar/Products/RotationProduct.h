#include "RadarProduct.h"

class RadarProductRotation : public RadarProduct {
public:
	RadarProductRotation();
	virtual RadarData* deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts) override;
};