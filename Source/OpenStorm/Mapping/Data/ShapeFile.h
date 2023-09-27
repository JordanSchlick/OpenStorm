#pragma once
#include <vector>
#include <string>
#include "../../Radar/SimpleVector3.h"

class GISObject {
public:
	// center in lat lon
	// SimpleVector3<float> center;
	
	// location in cartesian space on an un-oriented globe in meters
	SimpleVector3<float> location;
	// array pairs of latitude,longitude floats and separate parts are delineated by NaNs
	float* geometry = NULL;
	// number of floats in geometry buffer
	uint32_t geometryCount = 0;
	// if the object is currently show in game
	bool shown = false;
	// index of group that this object belongs to
	uint8_t groupId = 0;
	
	void Delete();
};

class GISGroup{
public:
	// distance in real world meters from camera to show objects
	float showDistance = 1000000;
	// width of lines
	float width = 20;
	float colorR = 1;
	float colorG = 1;
	float colorB = 1;
	GISGroup(float showDistance, float width){
		this->showDistance = showDistance;
		this->width = width;
	}
	GISGroup(float showDistance, float width, float r, float g, float b){
		this->showDistance = showDistance;
		this->width = width;
		this->colorR = r;
		this->colorG = g;
		this->colorB = b;
	}
};

bool ReadShapeFile(std::string fileName, std::vector<GISObject>* output, uint8_t groupId = 0);