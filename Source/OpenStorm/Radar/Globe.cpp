#include "Globe.h"
#define _USE_MATH_DEFINES
#include "math.h"

void Globe::SetCenter(double centerMetersX, double centerMetersY, double centerMetersZ){
	center.x = centerMetersX;
	center.y = centerMetersY;
	center.z = centerMetersZ;
}

// latitude and longitude in radians
void Globe::SetRotation(double latitudeRadians, double longitudeRadians){
	rotationAroundX = -latitudeRadians;
	rotationAroundPolls = longitudeRadians;
}

void Globe::SetRotationDegrees(double latitudeDegrees, double longitudeDegrees){
	SetRotation(latitudeDegrees / 180.0 * M_PI, longitudeDegrees / 180.0 * M_PI);
}

void Globe::SetTopCoordinates(double latitudeDegrees, double longitudeDegrees) {
	SetRotationDegrees(90.0 - latitudeDegrees, -longitudeDegrees);
}

double Globe::GetTopLatitudeDegrees() {
	return rotationAroundX / M_PI * 180.0 + 90.0;
}

double Globe::GetTopLongitudeDegrees() {
	return -rotationAroundPolls / M_PI * 180.0;
}

// get point in meters from latitude radians, longitude radians and altitude meters
SimpleVector3<> Globe::GetPoint(double latitudeRadians, double longitudeRadians, double altitude){
	SimpleVector3<> vector = SimpleVector3<>(altitude + surfaceRadius,  M_PI / 2.0 - longitudeRadians - rotationAroundPolls, M_PI/2 - latitudeRadians);
	vector.SphericalToRectangular();
	vector.RotateAroundX(rotationAroundX);
	vector.Add(center);
	return vector;
}

SimpleVector3<> Globe::GetPoint(SimpleVector3<> location){
	SimpleVector3<> vector = SimpleVector3<>(location.radius() + surfaceRadius,  M_PI / 2.0 - location.theta() - rotationAroundPolls, M_PI/2 - location.phi());
	vector.SphericalToRectangular();
	vector.RotateAroundX(rotationAroundX);
	vector.Add(center);
	return vector;
	return SimpleVector3<>();
}

SimpleVector3<> Globe::GetPointScaled(double latitudeRadians, double longitudeRadians, double altitude){
	SimpleVector3<> vector = GetPoint(latitudeRadians,longitudeRadians, altitude);
	vector.Multiply(scale);
	return vector;
}

SimpleVector3<> Globe::GetPointScaled(SimpleVector3<> location){
	SimpleVector3<> vector = GetPoint(location);
	vector.Multiply(scale);
	return vector;
}

SimpleVector3<> Globe::GetPointDegrees(double latitudeDegrees, double longitudeDegrees, double altitude) {
	SimpleVector3<> vector = GetPoint(latitudeDegrees / 180 * M_PI, longitudeDegrees / 180 * M_PI, altitude);
	return vector;
}

SimpleVector3<> Globe::GetPointScaledDegrees(double latitudeDegrees, double longitudeDegrees, double altitude) {
	SimpleVector3<> vector = GetPoint(latitudeDegrees / 180 * M_PI, longitudeDegrees / 180 * M_PI, altitude);
	vector.Multiply(scale);
	return vector;
}

SimpleVector3<> Globe::GetLocation(SimpleVector3<> point){
	SimpleVector3<> vector = SimpleVector3<>(point);
	vector.Add(center);
	vector.RotateAroundX(-rotationAroundX);
	vector.RectangularToSpherical();
	vector.radius() -= surfaceRadius;
	vector.theta() = -vector.theta() + M_PI / 2.0 - rotationAroundPolls;
	vector.phi() = -vector.phi() - M_PI / 2.0;
	return vector;
	return SimpleVector3<>();
}

SimpleVector3<> Globe::GetLocationScaled(SimpleVector3<> point){
	SimpleVector3<> vector = SimpleVector3<>(point);
	vector.Multiply(-1.0 / scale);
	vector = GetLocation(vector);
	return vector;
}