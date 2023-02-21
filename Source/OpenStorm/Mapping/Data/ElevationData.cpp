#include "ElevationData.h"
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <algorithm>
#include <fcntl.h>
#include <cstdlib>

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

inline float lerp(float x1, float x2, float amount){
	return x1 * (1.0f - amount) + x2 * amount;
}

namespace ElevationData{

	#define M_PI       3.14159265358979323846
	
	void* elevationData = NULL;
	int elevationDataWidth = 0;
	int elevationDataHeight = 0;
	int elevationDataType = 0;

	void LoadData(std::string dataFilePath) {
		if(elevationData != NULL){
			//return;
			delete[] elevationData;
			elevationData = NULL;
		}
		#ifdef _WIN32
			_set_fmode(_O_BINARY);
		#endif // _WIN32
		
		FILE* file = fopen(dataFilePath.c_str(), "r");
		// rows, columns, data type(0: float, 1: int16)
		int32_t shape[3];
		if(fread(shape, sizeof(int32_t), 3, file) != 3){
			fprintf(stderr, "Failed to read size of elevation data\n");
			fclose(file);
			return;
		}
		int dataSize = shape[0] * shape[1];
		if(shape[2] == 1){
			elevationData = new int16_t[dataSize];
		}else{
			elevationData = new float[dataSize];
		}
		
		if(elevationData == NULL || fread(elevationData, (shape[2] == 1) ? sizeof(int16_t) : sizeof(float), dataSize, file) != dataSize){
			fprintf(stderr, "Failed to read elevation data into buffer of size %i\n", dataSize);
			fclose(file);
			if(elevationData != NULL){
				delete[] elevationData;
				elevationData = NULL;
			}
			return;
		}
		elevationDataHeight = shape[0];
		elevationDataWidth = shape[1];
		elevationDataType = shape[2];
		fprintf(stderr, "Elevation size %i %i\n", elevationDataHeight, elevationDataWidth);
	}
	
	void UnloadData(){
		if(elevationData != NULL){
			delete[] elevationData;
			elevationData = NULL;
		}
		elevationDataHeight = 0;
		elevationDataWidth = 0;
	}
	
	
	inline float GetDataAtIndex(float x, float y){
		// sample data
		float val;
		float valUp;
		float valLeft;
		float valBoth;
		if(elevationDataType == 1){
			// int16 data type
			int16_t* elevationDataTyped = (int16_t*)elevationData;
			val = elevationDataTyped[
			elevationDataWidth * modulo((int)y, elevationDataHeight) + 
			modulo((int)x, elevationDataWidth)
			];
			valUp = elevationDataTyped[
				elevationDataWidth * modulo((int)y + 1, elevationDataHeight) + 
				modulo((int)x, elevationDataWidth)
			];
			valLeft = elevationDataTyped[
				elevationDataWidth * modulo((int)y, elevationDataHeight) + 
				modulo((int)x + 1, elevationDataWidth)
			];
			valBoth = elevationDataTyped[
				elevationDataWidth * modulo((int)y + 1, elevationDataHeight) + 
				modulo((int)x + 1, elevationDataWidth)
			];
		}else{
			// float data type
			float* elevationDataTyped = (float*)elevationData;
			val = elevationDataTyped[
				elevationDataWidth * modulo((int)y, elevationDataHeight) + 
				modulo((int)x, elevationDataWidth)
			];
			valUp = elevationDataTyped[
				elevationDataWidth * modulo((int)y + 1, elevationDataHeight) + 
				modulo((int)x, elevationDataWidth)
			];
			valLeft = elevationDataTyped[
				elevationDataWidth * modulo((int)y, elevationDataHeight) + 
				modulo((int)x + 1, elevationDataWidth)
			];
			valBoth = elevationDataTyped[
				elevationDataWidth * modulo((int)y + 1, elevationDataHeight) + 
				modulo((int)x + 1, elevationDataWidth)
			];
		}
		
		// interpolate
		val = lerp(
			lerp(val, valUp, fmod(y, 1.0)),
			lerp(valLeft, valBoth, fmod(y, 1.0)),
			fmod(x, 1.0)
		);
		return val;
	}
	
	float GetDataAtPointRadians(double latitude, double longitude){
		if(elevationData == NULL){
			return 0;
		}
		// flip and normalize
		latitude = std::fmod(-latitude + M_PI / 2.0, M_PI);
		if(latitude < 0){
			latitude += M_PI;
		}
		// normalize
		longitude = std::fmod(longitude + M_PI, M_PI * 2.0);
		if(longitude < 0){
			longitude += M_PI * 2.0;
		}
		// convert to buffer coordinates
		latitude = (latitude / M_PI) * elevationDataHeight;
		longitude = (longitude / M_PI / 2.0) * elevationDataWidth;
		
		return GetDataAtIndex(longitude, latitude);
	}
}