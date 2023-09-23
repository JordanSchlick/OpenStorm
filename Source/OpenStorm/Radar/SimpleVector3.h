#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

template<class T = double>
class SimpleVector3{
public:
	// x or radius
	T x;
	// y or theta
	T y;
	// z or phi
	T z;
	
	// alias for x
	inline T& radius() { return x; }
	// alias for y
    inline T& theta() { return y; }
	// alias for z
    inline T& phi() { return z; }
	// create zero vector
	inline SimpleVector3(){
		x = 0;
		y = 0;
		z = 0;
	}
	// create new vector
	inline SimpleVector3(T startingX,T startingY,T startingZ){
		x = startingX;
		y = startingY;
		z = startingZ;
	}
	// duplicate another vector
	template <class T2>
	SimpleVector3(const SimpleVector3<T2> &from){
		x = from.x;
		y = from.y;
		z = from.z;
	}
	// convert from spherical to cartesian
	inline void SphericalToRectangular(){
		T recX = x * sin(z) * cos(y);
		T recY = x * sin(z) * sin(y);
		T recZ = x * cos(z);
		x = recX;
		y = recY;
		z = recZ;
	}
	// convert from cartesian to spherical
	inline void RectangularToSpherical(){
		T radius = sqrt(x * x + y * y + z * z);
		T theta = atan2(y, x);
		T phi = acos(z / radius);
		x = radius;
		y = theta;
		z = phi;
	}
	// add another vector from this one
	inline void Add(const SimpleVector3 &other){
		x += other.x;
		y += other.y;
		z += other.z;
	}
	// subtract another vector from this one
	inline void Subtract(const SimpleVector3 &other){
		x -= other.x;
		y -= other.y;
		z -= other.z;
	}
	// multiply by scaler
	inline void Multiply(T scaler) {
		x *= scaler;
		y *= scaler;
		z *= scaler;
	}
	// elementwise multiplication
	inline void Multiply(const SimpleVector3 &other) {
		x *= other.x;
		y *= other.y;
		z *= other.z;
	}
	// cross product
	inline void Cross(const SimpleVector3 &other) {
		T newX = y * other.z - z * other.y;
		T newY = z * other.x - x * other.z;
		T newZ = x * other.y - y * other.x;
		x = newX;
		y = newY;
		z = newZ;
	}
	// cross product with other vector first
	inline void CrossOther(const SimpleVector3 &other) {
		T newX = other.y * z - other.z * y;
		T newY = other.z * x - other.x * z;
		T newZ = other.x * y - other.y * x;
		x = newX;
		y = newY;
		z = newZ;
	}
	// project onto vector
	inline void Project(const SimpleVector3 &onto){
		float scale = Dot(onto) / onto.Dot(onto);
		x = onto.x * scale;
		y = onto.y * scale;
		z = onto.z * scale;
	}
	// project onto plane based on normal vector
	inline void ProjectOntoPlane(const SimpleVector3 &planeNormal){
		T oldX = x;
		T oldY = y;
		T oldZ = z;
		Project(planeNormal);
		x = oldX - x;
		y = oldY - y;
		z = oldZ - z;
	}
	inline void Normalize(){
		T magnitude = Magnitude();
		x /= magnitude;
		y /= magnitude;
		z /= magnitude;
	}
	// rotate around x axis
	inline void RotateAroundX(T rotationRadians) {
		T newY = y * cos(rotationRadians) + z * sin(rotationRadians);
		T newZ = -y * sin(rotationRadians) + z * cos(rotationRadians);
		y = newY;
		z = newZ;
	}
	// rotate around y axis
	inline void RotateAroundY(T rotationRadians) {
		T newX = x * cos(rotationRadians) + z * sin(rotationRadians);
		T newZ = -x * sin(rotationRadians) + z * cos(rotationRadians);
		x = newX;
		z = newZ;
	}
	// rotate around z axis
	inline void RotateAroundZ(T rotationRadians) {
		T newX = x * cos(rotationRadians) + y * sin(rotationRadians);
		T newY = -x * sin(rotationRadians) + y * cos(rotationRadians);
		x = newX;
		y = newY;
	}
	// dot product
	inline T Dot(const SimpleVector3 &other) const{
		return x * other.x + y * other.y + z * other.z;
	}
	// get length of vector
	inline T Magnitude() const{
		return sqrt(x * x + y * y + z * z);
	}
	// get distance between this and other vector
	inline T Distance(const SimpleVector3 &other) const{
		T xDelta = x - other.x;
		T yDelta = y - other.y;
		T zDelta = z - other.z;
		return sqrt(xDelta * xDelta + yDelta * yDelta + zDelta * zDelta);
	}
	// elementwise addition
	inline SimpleVector3<T> operator+(const SimpleVector3<T> &other) const{
		SimpleVector3<T> newVector = SimpleVector3<T>(*this);
		newVector.Add(other);
		return newVector;
	}
	// elementwise subtraction
	inline SimpleVector3<T> operator-(const SimpleVector3<T> &other) const{
		SimpleVector3<T> newVector = SimpleVector3<T>(*this);
		newVector.Subtract(other);
		return newVector;
	}
};



