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
	
	float* elevationData = NULL;
	int elevationDataWidth = 0;
	int elevationDataHeight = 0;

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
		int32_t shape[2];
		if(fread(shape, sizeof(int32_t), 2, file) != 2){
			fprintf(stderr, "Failed to read size of elevation data\n");
			fclose(file);
			return;
		}
		int dataSize = shape[0] * shape[1];
		elevationData = new float[dataSize];
		if(elevationData == NULL || fread(elevationData, sizeof(float), dataSize, file) != dataSize){
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
		fprintf(stderr, "Elevation size %i %i\n", elevationDataHeight, elevationDataWidth);
	}

	float GetDataAtPointRadians(double latitude, double longitude){
		if(elevationData == NULL){
			return 0;
		}
		latitude = std::fmod(latitude + M_PI / 2.0, M_PI);
		if(latitude < 0){
			latitude += M_PI;
		}
		longitude = std::fmod(longitude + M_PI, M_PI * 2.0);
		if(longitude < 0){
			longitude += M_PI * 2.0;
		}
		latitude = (latitude / M_PI) * elevationDataHeight;
		longitude = (longitude / M_PI / 2.0) * elevationDataWidth;
		float val = elevationData[
			elevationDataWidth * modulo((int)latitude, elevationDataHeight) + 
			modulo((int)longitude, elevationDataWidth)
		];
		float valUp = elevationData[
			elevationDataWidth * modulo((int)latitude + 1, elevationDataHeight) + 
			modulo((int)longitude, elevationDataWidth)
		];
		float valLeft = elevationData[
			elevationDataWidth * modulo((int)latitude, elevationDataHeight) + 
			modulo((int)longitude + 1, elevationDataWidth)
		];
		float valBoth = elevationData[
			elevationDataWidth * modulo((int)latitude + 1, elevationDataHeight) + 
			modulo((int)longitude + 1, elevationDataWidth)
		];
		val = lerp(
			lerp(val, valUp, fmod(latitude, 1.0)),
			lerp(valLeft, valBoth, fmod(latitude, 1.0)),
			fmod(longitude, 1.0)
		);
		return val;
	}
}