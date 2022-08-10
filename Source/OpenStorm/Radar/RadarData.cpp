
#include "RadarData.h"
#include "SparseCompression.h"
#include "./Deps/rsl/rsl.h"



//#include "CoreMinimal.h"
//#include "Engine/Texture2D.h"

#ifdef __INTELLISENSE__
// visual studio may have a fucking meltdown if it includes c++ standard headers move them into the else block here if it happens
#else

#endif
#include <map>
#include <algorithm>
#include <cmath>



// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}



void RadarData::ReadNexrad(const char* filename) {

	//RSL_wsr88d_keep_sails();
	//Radar* radar = RSL_wsr88d_to_radar("C:/Users/Admin/Desktop/stuff/projects/openstorm/files/KMKX_20220723_235820", "KMKX");
	//Radar* radar = RSL_wsr88d_to_radar("C:/Users/Admin/Desktop/stuff/projects/openstorm/files/KMKX_20220723_233154", "KMKX");
	Radar* radar = RSL_wsr88d_to_radar((char*)filename, (char*)"");

	//UE_LOG(LogTemp, Display, TEXT("================ %i"), radar);

	

	if (radar) {
		Volume* volume = radar->v[DZ_INDEX];
		if (volume) {

			

			std::map<float, Sweep*> sweeps = {};

			fprintf(stderr, "volume type_str %s\n", volume->h.type_str);
			fprintf(stderr, "volume nsweeps %i\n", volume->h.nsweeps);
			for (int sweepIndex = 0; sweepIndex < volume->h.nsweeps; sweepIndex++) {
				Sweep* sweep = volume->sweep[sweepIndex];
				if (sweep) {
					if (!sweep->ray[0]) {
						// should not happen
						fprintf(stderr, "Ray is missing!\n");
						continue;
					}
					/*fprintf(stderr, "======== %i\n", sweepIndex);
					fprintf(stderr, "sweep elev %f\n", sweep->h.elev);
					fprintf(stderr, "sweep sweep_num %i\n", sweep->h.sweep_num);
					fprintf(stderr, "sweep nrays %i\n", sweep->h.nrays);
					if (sweep->ray[0]) {
						fprintf(stderr, "sweep nbins %i\n", sweep->ray[0]->h.nbins);
					}
					fprintf(stderr, "sweep azimuth %f\n", sweep->h.azimuth);
					fprintf(stderr, "sweep beam_width %f\n", sweep->h.beam_width);
					fprintf(stderr, "sweep horz_half_bw %f\n", sweep->h.horz_half_bw);
					fprintf(stderr, "sweep vert_half_bw %f\n", sweep->h.vert_half_bw);
					if (sweep->ray[0]) {
						fprintf(stderr, "sweep bin 0 data %i\n", sweep->ray[0]->range[0]);
						fprintf(stderr, "sweep ray pixel length %i meters\n", sweep->ray[0]->h.gate_size);
						fprintf(stderr, "sweep ray estimated length %i meters\n", sweep->ray[0]->h.gate_size * sweep->ray[0]->h.nbins + sweep->ray[0]->h.range_bin1);
						fprintf(stderr, "sweep ray unam_rng %f meters\n", sweep->ray[0]->h.unam_rng);
					}*/

					

					// add sweeps but skip duplicates
					float roundedElevation = std::round(sweep->h.elev * 100.0f) / 100.0f;
					if (sweeps.find(roundedElevation) == sweeps.end()) {
						sweeps[roundedElevation] = sweep;
					}
				}
			}



			
			if (sweepBufferCount == 0) {
				sweepBufferCount = sweeps.size();
			}
			if (sweepInfo != NULL) {
				delete[] sweepInfo;
			}
			sweepInfo = new SweepInfo[sweepBufferCount]();
			int sweepId = 0;
			int maxTheta = 0;
			int maxRadius = 0;
			minValue = INFINITY;
			maxValue = -INFINITY;
			
			// do a pass of the data to find info
			for (const auto pair : sweeps) {
				if (sweepId >= sweepBufferCount) {
					break;
				}
				Sweep* sweep = pair.second;
				sweepInfo[sweepId].id = sweepId;
				sweepInfo[sweepId].elevation = sweep->h.elev;
				sweepInfo[sweepId].actualRayCount = sweep->h.nrays;
				innerDistance = (float)sweep->ray[0]->h.range_bin1 / (float)sweep->ray[0]->h.gate_size;
				//fprintf(stderr, "innerDistance: %f\n", innerDistance);

				int thetaSize = sweep->h.nrays;
				maxTheta = std::max(maxTheta, thetaSize);
				for (int theta = 0; theta < thetaSize; theta++) {
					Ray* ray = sweep->ray[theta];
					
					if (ray) {
						int radiusSize = ray->h.nbins;
						maxRadius = std::max(maxRadius, radiusSize);
						for (int radius = 0; radius < radiusSize; radius++) {
							int value = ray->h.f(ray->range[radius]);
							//int value = ray->range[radius];
							if (value != 131072) {
								minValue = value != 0 ? (value < minValue ? value : minValue) : minValue;
								//minValue = value < minValue ? value : minValue;
								maxValue = value > maxValue ? value : maxValue;
							}
						}
					}
				}

				sweepId++;
			}
			
			if (minValue == INFINITY) {
				minValue = 0;
				maxValue = 1;
			}
			if (radiusBufferCount == 0) {
				radiusBufferCount = maxRadius;
			}
			if (thetaBufferCount == 0) {
				thetaBufferCount = maxTheta;
			}


			fprintf(stderr, "min: %f   max: %f\n", minValue, maxValue);
			
			// sizes of sections of the buffer
			thetaBufferSize = radiusBufferCount;
			sweepBufferSize = (thetaBufferCount + 2) * thetaBufferSize;
			fullBufferSize = sweepBufferCount * sweepBufferSize;
			
			SparseCompress::CompressorState compressorState = {};
			
			// pointer to beginning of a sweep buffer to write to
			float* sweepBuffer = NULL;
			
			if(compress){
				// store in compressed form
				compressorState.preCompressedSize = fullBufferSize / 10;
				SparseCompress::compressStart(&compressorState);
				sweepBuffer = new float[sweepBufferSize];
			}else{
				// store in continuous buffer
				if (buffer == NULL) {
					buffer = new float[fullBufferSize];
					std::fill(buffer, buffer+fullBufferSize, -INFINITY);
				}
			}
			
			int sweepIndex = 0;
			for (const auto pair : sweeps) {
				if (sweepIndex >= sweepBufferCount) {
					// ignore sweeps that wont fit in buffer
					break;
				}
				Sweep* sweep = pair.second;
				int thetaSize = sweep->h.nrays;
				int sweepOffset = sweepIndex * sweepBufferSize;
				if(compress){
					std::fill(sweepBuffer, sweepBuffer + sweepBufferSize, -INFINITY);
				}else{
					sweepBuffer = buffer + sweepOffset;
				}
				bool* usedThetas = new bool[thetaBufferCount]();
				// fill in buffer from rays
				for (int theta = 0; theta < thetaSize; theta++) {
					Ray* ray = sweep->ray[theta];
					if (ray) {
						// get real angle of ray
						int realTheta = (int)((ray->h.azimuth * 2.0) + thetaBufferCount) % thetaBufferCount;
						usedThetas[realTheta] = true;
						int radiusSize = std::min(ray->h.nbins, radiusBufferCount);
						for (int radius = 0; radius < radiusSize; radius++) {
							//int value = (ray->range[radius] - minValue) / divider;
							float value = ray->h.f(ray->range[radius]);
							//float value = ray->range[radius];
							if (value == 131072) {
								// value for no data
								value = -INFINITY;
							}
							sweepBuffer[radius + ((realTheta + 1) * thetaBufferSize)] = value;

							//if (theta == 0) {
							//	value = 255;
							//}
							//RawImageData[3 + radius * 4 + realTheta * radiusBufferSize + sweepIndex * thetaRadiusBufferSize] = std::max(0, value);
							//RawImageData[3 + radius * 4 + theta * radiusSize * 4] = 0;
							//setBytes++;
						}
					}
					//break;
				}
				

				for (int theta = 0; theta < thetaBufferCount; theta++) {
					if (!usedThetas[theta]) {
						// fill in blank by interpolating surroundings
						int previousRay = 0;
						int nextRay = 0;
						// find 2 nearby populated rays
						if (usedThetas[modulo(theta - 3, thetaBufferCount)]) {
							previousRay = -3;
						}
						if (usedThetas[modulo(theta - 2, thetaBufferCount)]) {
							previousRay = -2;
						}
						if (usedThetas[modulo(theta - 1, thetaBufferCount)]) {
							previousRay = -1;
						}
						if (usedThetas[modulo(theta + 3, thetaBufferCount)]) {
							nextRay = 4;
						}
						if (usedThetas[modulo(theta + 2, thetaBufferCount)]) {
							nextRay = 2;
						}
						if (usedThetas[modulo(theta + 1, thetaBufferCount)]) {
							nextRay = 1;
						}
						if (previousRay != 0 && nextRay != 0) {
							int previousRayAbs = modulo(theta + previousRay, thetaBufferCount);
							int nextRayAbs = modulo(theta + nextRay, thetaBufferCount);
							for (int radius = 0; radius < radiusBufferCount; radius++) {
								float previousValue = sweepBuffer[radius + (previousRayAbs + 1) * thetaBufferSize];
								float nextValue = sweepBuffer[radius + (nextRayAbs + 1) * thetaBufferSize];
								for (int thetaTo = previousRay + 1; thetaTo <= nextRay - 1; thetaTo++) {
									float interLocation = (float)(thetaTo - previousRay) / (float)(nextRay - previousRay);
									// write interpolated value
									sweepBuffer[radius + (modulo(theta + thetaTo, thetaBufferCount) + 1) * thetaBufferSize] = previousValue * (1.0 - interLocation) + nextValue * interLocation;
								}
							}
							for (int thetaTo = previousRay + 1; thetaTo <= nextRay - 1; thetaTo++) {
								// mark as filled
								usedThetas[modulo(theta + thetaTo, thetaBufferCount)] = true;
							}
						}
					}
				}
				delete[] usedThetas;
				
				// pad theta with ray from other side to allow correct interpolation in shader at edges
				memcpy(
					sweepBuffer,
					sweepBuffer + (thetaBufferCount * thetaBufferSize),
					thetaBufferSize*4);
				memcpy(
					sweepBuffer + (((thetaBufferCount + 1) * thetaBufferSize)),
					sweepBuffer + (thetaBufferSize),
					thetaBufferSize*4);
				
				if(compress){
					SparseCompress::compressValues(&compressorState, sweepBuffer, sweepBufferSize);
				}
				
				sweepIndex++;
			}
			
			if(compress){
				delete sweepBuffer;
				if(bufferCompressed){
					delete[] bufferCompressed;
				}
				bufferCompressed = SparseCompress::compressEnd(&compressorState);
				compressedBufferSize = compressorState.sizeAllocated;
				fprintf(stderr, "Compressed size bytes:   %i\n", compressedBufferSize * 4);
				fprintf(stderr, "Uncompressed size bytes: %i\n", fullBufferSize * 4);
			}
			

			/*
			fprintf(stderr,"ray values:");
			for(int radius = 0; radius < thetaBufferSize; radius++){
				fprintf(stderr," %.10f",buffer[radius + sweepBufferSize * 2]);
			}
			fprintf(stderr,"\n");
			//*/

			RSL_free_radar(radar);
		}
	}
}

void RadarData::CopyFrom(RadarData* data) {
	if(buffer == NULL){
		if (radiusBufferCount == 0) {
			radiusBufferCount = data->radiusBufferCount;
		}
		if (thetaBufferCount == 0) {
			thetaBufferCount = data->thetaBufferCount;
		}
		if (sweepBufferCount == 0) {
			sweepBufferCount = data->sweepBufferCount;
		}
		
		thetaBufferSize = radiusBufferCount;
		sweepBufferSize = (thetaBufferCount + 2) * thetaBufferSize;
		fullBufferSize = sweepBufferCount * sweepBufferSize;
		
		buffer = new float[fullBufferSize];
		std::fill(buffer, buffer+fullBufferSize, -INFINITY);
	}
	if(data->bufferCompressed != NULL){
		SparseCompress::decompressToBuffer(buffer, data->bufferCompressed, fullBufferSize);
	}else if(data->buffer != NULL){
		memcpy(buffer, data->buffer, std::min(fullBufferSize, data->fullBufferSize) * 4);
	}
}



RadarData::TextureBuffer RadarData::CreateTextureBufferReflectivity() {
	int thetaBufferSize2 = radiusBufferCount;
	int sweepBufferSize2 = (thetaBufferCount + 2) * thetaBufferSize2;
	int fullBufferSize2 = sweepBufferCount * sweepBufferSize2;
	
	
	if(buffer == NULL){
		RadarData::TextureBuffer returnValue;
		returnValue.data = NULL;
		//returnValue.data = new float[fullBufferSize2];
		returnValue.byteSize = 0;
		return returnValue;
	}
	
	// sizes of secitions of the buffer
	

	float* textureBuffer = new float[fullBufferSize2]();
	//int divider = (maxValue - minValue) / 256 + 1;
	float divider = (maxValue - minValue);
	for (int sweepIndex = 0; sweepIndex < sweepBufferCount; sweepIndex++) {
		for (int theta = 0; theta < thetaBufferCount; theta++) {
			for (int radius = 0; radius < radiusBufferCount; radius++) {
				//float value = (buffer[radius + (theta * radiusBufferCount) + (sweepIndex * radiusBufferCount * thetaBufferCount)] - minValue) / divider;
				//textureBuffer[(radius) + ((theta + 1) * thetaBufferSize) + (sweepIndex * sweepBufferSize)] = std::max(0.0f, value);
				
				float value = buffer[radius + (theta * radiusBufferCount) + (sweepIndex * radiusBufferCount * thetaBufferCount)];
				textureBuffer[(radius) + ((theta + 1) * thetaBufferSize) + (sweepIndex * sweepBufferSize)] =  value;
				//if (theta == 0) {
				//	value = 255;
				//}
				
			}
		}
		// pad theta with pixels from the other side
		//fprintf(stderr, "sweepIndex %i \n", sweepIndex);
		//fflush(stderr);
		memcpy(
			textureBuffer + (sweepIndex * sweepBufferSize2),
			textureBuffer + (thetaBufferCount * thetaBufferSize2 + (sweepIndex * sweepBufferSize2)),
			thetaBufferSize2*4);

		


		memcpy(
			textureBuffer + (((thetaBufferCount + 1) * thetaBufferSize2) + (sweepIndex * sweepBufferSize2)),
			textureBuffer + (thetaBufferSize2 + (sweepIndex * sweepBufferSize2)),
			thetaBufferSize2*4);

		/*if (sweepIndex == 3 ) {
			//for (int radius = 0; radius < thetaBufferSize; radius++) {
			//	textureBuffer[(sweepIndex * sweepBufferSize) + radius] = 255;
			//}
			for (int radius = 0; radius < thetaBufferSize; radius++) {
				//textureBuffer[((230) * thetaBufferSize) + (sweepIndex * sweepBufferSize) + radius] = 255;
				textureBuffer[((230) * thetaBufferSize) + (sweepIndex * sweepBufferSize) + radius] = 1.0f * (float)radius / (float)thetaBufferSize;
			}
		}*/
		/*memcpy(textureBuffer + (sweepIndex * thetaRadiusBufferSize), textureBuffer + (radiusBufferCount * radiusBufferSize + sweepIndex * thetaRadiusBufferSize), radiusBufferSize);
		for (int radius = 0; radius < radiusBufferCount; radius++) {
			textureBuffer[3 + radius * 4 + sweepIndex * thetaRadiusBufferSize] = textureBuffer[3 + radius * 4 + radiusBufferCount * radiusBufferSize + sweepIndex * thetaRadiusBufferSize];
			textureBuffer[3 + radius * 4 + thetaBufferCount * radiusBufferSize + sweepIndex * thetaRadiusBufferSize] = textureBuffer[3 + radius * 4 + radiusBufferSize + sweepIndex * thetaRadiusBufferSize];
		}*/
	}
	//for(int sweep)
	RadarData::TextureBuffer returnValue;
	returnValue.data = textureBuffer;
	returnValue.byteSize = fullBufferSize2 * 4;
	return returnValue;
}

RadarData::TextureBuffer RadarData::CreateTextureBufferReflectivity2(){
	RadarData::TextureBuffer returnValue;
	if(bufferCompressed != NULL){
		returnValue.data = bufferCompressed;
		returnValue.byteSize = compressedBufferSize;
		return returnValue;
	}
	if(buffer == NULL){
		returnValue.data = NULL;
		returnValue.byteSize = 0;
		return returnValue;
	}
	returnValue.data = buffer;
	returnValue.byteSize = fullBufferSize * 4;
	return returnValue;
}


RadarData::TextureBuffer RadarData::CreateAngleIndexBuffer() {
	float* textureBuffer = new float[65536]();
	int divider = (maxValue - minValue) / 256 + 1;
	for (int sweepIndex = 1; sweepIndex < sweepBufferCount; sweepIndex++) {
		RadarData::SweepInfo info1 = sweepInfo[sweepIndex - 1];
		RadarData::SweepInfo info2 = sweepInfo[sweepIndex];
		if(info2.id == -1){
			break;
		}
		int startLocation = info1.elevation * 32768 / 90 + 32768;
		int endLocation = info2.elevation * 32768 / 90 + 32768;
		int delta = endLocation - startLocation;
		float deltaF = delta;
		//fprintf(stderr, "delta: %i %f\n", delta, info1.elevation);
		float firstSweepIndex = sweepIndex - 1.0f;
		for (int i = 0; i <= delta; i++) {
			float subLocation = (float)i / deltaF;
			textureBuffer[startLocation + i] = firstSweepIndex + subLocation;
		}
	}

	/*for (int i = 0; i < 65536; i++) {
		if (i >= 32768) {
			float sweep = ((i - 32768) * 32) / 32768.0;
			//if (!((int)sweep - sweep < 0.2 && sweep - (int)sweep < 0.2)) {
			//	sweep = 0;
			//}
			textureBuffer[i] = sweep;
		} else {
			textureBuffer[i] = 0;
		}
		//rawAngleIndexImageData[i] = 0;
	}*/
	RadarData::TextureBuffer returnValue;
	returnValue.data = textureBuffer;
	returnValue.byteSize = 65536 * 4;
	return returnValue;
}

void RadarData::Compress() {
	if(buffer == NULL){
		return;
	}
	int compressedSize = 2;
	bool isEmpty = true;
	for(int i = 0; i < fullBufferSize; i++){
		float value = buffer[i];
		if(value == -INFINITY){
			if(isEmpty == false){
				compressedSize += 2;
				isEmpty = true;
			}
		}else{
			compressedSize++;
			isEmpty = false;
		}
	}
	fprintf(stderr, "Compressed size bytes:   %i\n", compressedSize * 4);
	fprintf(stderr, "Uncompressed size bytes: %i\n", fullBufferSize * 4);
}

void RadarData::Deallocate(){
	if(buffer != NULL){
		delete[] buffer;
		buffer = NULL;
	}
	if(bufferCompressed != NULL){
		delete[] bufferCompressed;
		bufferCompressed = NULL;
	}
	if(sweepInfo != NULL){
		delete[] sweepInfo;
		sweepInfo = NULL;
	}
}


RadarData::~RadarData(){
	RadarData::Deallocate();
	fprintf(stderr,"freed RadarData\n");
}