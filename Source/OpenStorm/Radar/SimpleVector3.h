#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

template<class T = double>
class SimpleVector3{
public:
	// x or radius
	T x = 0;
	// y or theta
	T y = 0;
	// z or phi
	T z = 0;
	
	// alias for x
	inline T& radius() { return x; }
	// alias for y
    inline T& theta() { return y; }
	// alias for z
    inline T& phi() { return z; }
	
	SimpleVector3(){}
	SimpleVector3(T startingX,T startingY,T startingZ){
		x = startingX;
		y = startingY;
		z = startingZ;
	}
	void SphericalToRectangular(){
		T recX = x * sin(z) * cos(y);
		T recY = x * sin(z) * sin(y);
		T recZ = x * cos(z);
		x = recX;
		y = recY;
		z = recZ;
	}
	void RectangularToSpherical(){
		T radius = sqrt(x * x + y * y + z * z);
		T theta = atan2(-y, -x);
		T phi = acos(z / radius);
		x = radius;
		y = theta;
		z = phi;
	}
	void Add(SimpleVector3 &other){
		x += other.x;
		y += other.y;
		z += other.z;
	}
	void Subtract(SimpleVector3 &other){
		x -= other.x;
		y -= other.y;
		z -= other.z;
	}
	T Dot(SimpleVector3 &other){
		x * other.x + y * other.y + z * other.z;
	}
	void Multiply(T scaler) {
		x *= scaler;
		y *= scaler;
		z *= scaler;
	}
	void Multiply(SimpleVector3 &other) {
		x *= other.x;
		y *= other.y;
		z *= other.z;
	}
	void RotateAroundX(T rotationRadians) {
		T newY = y * cos(rotationRadians) + z * sin(rotationRadians);
		T newZ = -y * sin(rotationRadians) + z * cos(rotationRadians);
		y = newY;
		z = newZ;
	}
	void RotateAroundY(T rotationRadians) {
		T newX = x * cos(rotationRadians) + z * sin(rotationRadians);
		T newZ = -x * sin(rotationRadians) + z * cos(rotationRadians);
		x = newX;
		z = newZ;
	}
	void RotateAroundZ(T rotationRadians) {
		T newX = x * cos(rotationRadians) + y * sin(rotationRadians);
		T newY = -x * sin(rotationRadians) + y * cos(rotationRadians);
		x = newX;
		y = newY;
	}
	T Magnitude(){
		return sqrt(x * x + y * y + z * z);
	}
};



