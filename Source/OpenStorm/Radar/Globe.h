#pragma once

#include "SimpleVector3.h"

// Represents the earth in meters and radian
class Globe{
public:
	

	// scale between real life and in game units
	double scale = 1.0 / 10000.0 * 100.0;
	
	// radius of the surface of the globe
	double surfaceRadius = 6371000.0;
	
	// use SetCenter to change
	SimpleVector3<> center = SimpleVector3<>(0.0, 0.0, 0.0);
	
	// use SetRotation to change
	double rotationTheta = 0;
	double rotationPhi = 0;
	
	// set center in meters from origin
	void SetCenter(double centerMetersX, double centerMetersY, double centerMetersZ);
	
	// latitude and longitude in radians to rotate the globe
	void SetRotation(double latitudeRadians, double longitudeRadians);
	
	// latitude and longitude in degrees to rotate the globe
	void SetRotationDegrees(double latitudeDegrees, double longitudeDegrees);
	
	// set the rotation so that the coordinates that will be at the top of the globe
	void SetTopCoordinates(double latitudeDegrees, double longitudeDegrees);
	
	// returns the latitude in degrees that is at the top based on the rotation
	double GetTopLatitudeDegrees();
	
	// returns the longitude in degrees that is at the top based on the rotation
	double GetTopLongitudeDegrees();
	
	// get point in meters from latitude radians, longitude radians and altitude meters
	SimpleVector3<> GetPoint(double latitudeRadians, double longitudeRadians, double altitude);
	
	// get point in game units from latitude radians, longitude radians and altitude meters
	SimpleVector3<> GetPointScaled(double latitudeRadians, double longitudeRadians, double altitude);
	
	// get point in game units from latitude degrees, longitude degrees and altitude meters
	SimpleVector3<> GetPointScaledDegrees(double latitudeDegrees, double longitudeDegrees, double altitude);
	
	// get point in meters from latitude degrees, longitude degrees and altitude meters
	SimpleVector3<> GetPointDegrees(double latitudeDegrees, double longitudeDegrees, double altitude);
	
};