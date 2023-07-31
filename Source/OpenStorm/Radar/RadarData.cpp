
#include "RadarData.h"
#include "SparseCompression.h"
#include "NexradSites/NexradSites.h"
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
#include <ctime>

#define PIF 3.14159265358979323846f

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

// compares if argument larger is larger than argument smaller within a looped space
inline bool moduloCompare(int smaller, int larger, int n) {
	int offset = n/2 - smaller;
	return modulo(smaller + offset,n) < modulo(larger + offset,n);
}

// compares if smaller is closer to zero than larger within a looped space
inline bool moduloSmallerAbs(int smaller, int larger, int n) {
	return std::min(modulo(smaller,n), modulo(-smaller,n)) < std::min(modulo(larger,n), modulo(-larger,n));
}

// returns first - second where the result is at most n/2
inline int moduloSignedSubtract(int first, int second, int n){
	int result = modulo(first - second, n);
	if(result > n/2){
		result -= n;
	}
	return result;
}

bool RadarData::verbose = false;

void* RadarData::ReadNexradData(const char* filename) {
	//RSL_wsr88d_keep_sails();
	//Radar* radar = RSL_wsr88d_to_radar("C:/Users/Admin/Desktop/stuff/projects/openstorm/files/KMKX_20220723_235820", "KMKX");
	//Radar* radar = RSL_wsr88d_to_radar("C:/Users/Admin/Desktop/stuff/projects/openstorm/files/KMKX_20220723_233154", "KMKX");
	RSL_wsr88d_keep_sails();
	Radar* radar = RSL_wsr88d_to_radar((char*)filename, (char*)"");

	//UE_LOG(LogTemp, Display, TEXT("================ %i"), radar);
	
	if(verbose && radar){
		for(int i = 0; i <= 46; i++){
			Volume* volume = radar->v[i];
			if(volume){
				fprintf(stderr, "Volume %i %s\n", i, volume->h.type_str);
			}
		}
	}
	
	return radar;
}

void RadarData::FreeNexradData(void* nexradData) {
	if (nexradData) {
		RSL_free_radar((Radar*)nexradData);
	}
}

bool RadarData::LoadNexradVolume(void* nexradData, VolumeType volumeType) {
	Radar* radar = (Radar*)nexradData;
	if (radar) {
		int nexradType = DZ_INDEX;
		switch (volumeType){
			case VOLUME_REFLECTIVITY:
				nexradType = DZ_INDEX;
				break;
			case VOLUME_VELOCITY:
				nexradType = VR_INDEX;
				stats.noDataValue = 0;
				break;
			case VOLUME_SPECTRUM_WIDTH:
				nexradType = SW_INDEX;
				break;
			case VOLUME_CORELATION_COEFFICIENT:
				nexradType = RH_INDEX;
				break;
			case VOLUME_DIFFERENTIAL_REFLECTIVITY:
				nexradType = DR_INDEX;
				break;
			case VOLUME_DIFFERENTIAL_PHASE_SHIFT:
				nexradType = PH_INDEX;
				break;
			default:
				return false;
				break;
		}
		Volume* volume = radar->v[nexradType];
		if(verbose){
			fprintf(stderr, "site name %s\n", radar->h.name);
		}
		NexradSites::Site* site = NexradSites::GetSite(radar->h.name);
		if(site != NULL){
			stats.latitude = site->latitude;
			stats.longitude = site->longitude;
			stats.altitude = site->altitude;
		}
		if (volume) {
			stats.volumeType = volumeType;
			std::map<float, Sweep*> sweeps = {};
			if(verbose){
				fprintf(stderr, "volume type_str %s\n", volume->h.type_str);
				fprintf(stderr, "volume nsweeps %i\n", volume->h.nsweeps);
			}
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
						fprintf(stderr, "sweep ray lat %f lon %f\n", sweep->ray[1]->h.lat, sweep->ray[1]->h.lon);
					}
					fprintf(stderr, "%i-%i-%i %i:%i:%f\n", sweep->ray[0]->h.year, sweep->ray[0]->h.month, sweep->ray[0]->h.day, sweep->ray[0]->h.hour, sweep->ray[0]->h.minute, sweep->ray[0]->h.sec);
					*/
					
					
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
			sweepInfo = new SweepInfo[sweepBufferCount]{};
			int sweepId = 0;
			int maxTheta = 0;
			int maxRadius = 0;
			stats.minValue = INFINITY;
			stats.maxValue = -INFINITY;
			stats.boundUpper = 0;
			stats.boundLower = 1e-10;
			stats.boundRadius = 0;
			stats.beginTime = INFINITY;
			stats.endTime = -INFINITY;
			double lastRayDate = 0;
			
			// find info about sweeps
			for (const auto pair : sweeps) {
				if (sweepId >= sweepBufferCount) {
					// ignore sweeps that wont fit in buffer
					break;
				}
				Sweep* sweep = pair.second;
				sweepInfo[sweepId].id = sweepId;
				sweepInfo[sweepId].elevationAngle = sweep->h.elev;
				sweepInfo[sweepId].actualRayCount = sweep->h.nrays;
				stats.innerDistance = (float)sweep->ray[0]->h.range_bin1 / (float)sweep->ray[0]->h.gate_size;
				stats.pixelSize = (float)sweep->ray[0]->h.gate_size;
				//fprintf(stderr, "innerDistance: %f\n", innerDistance);

				int thetaSize = sweep->h.nrays;
				maxTheta = std::max(maxTheta, thetaSize);

				sweepId++;
			}
			
			//fprintf(stderr, "bounds %f %f %f\n",stats.boundRadius,stats.boundUpper,stats.boundLower);
			
			if (stats.minValue == INFINITY) {
				stats.minValue = 0;
				stats.maxValue = 1;
			}
			if (stats.beginTime == INFINITY) {
				stats.beginTime = 0;
				stats.endTime = 0;
			}
			if (radiusBufferCount == 0) {
				radiusBufferCount = maxRadius;
			}
			if (thetaBufferCount == 0) {
				thetaBufferCount = maxTheta;
			}

			if(verbose){
				fprintf(stderr, "min: %f   max: %f\n", stats.minValue, stats.maxValue);
			}
			
			// sizes of sections of the buffer
			thetaBufferSize = radiusBufferCount;
			sweepBufferSize = (thetaBufferCount + 2) * thetaBufferSize;
			fullBufferSize = sweepBufferCount * sweepBufferSize;
			
			if (rayInfo != NULL) {
				delete[] rayInfo;
			}
			rayInfo = new RayInfo[sweepBufferCount * (thetaBufferCount + 2)]{};
			
			float noDataValue = stats.noDataValue;
			
			SparseCompress::CompressorState compressorState = {};
			
			// pointer to beginning of a sweep buffer to write to
			float* sweepBuffer = NULL;
			
			if(doCompress){
				// store in compressed form
				compressorState.preCompressedSize = fullBufferSize / 10;
				compressorState.emptyValue = stats.noDataValue;
				SparseCompress::compressStart(&compressorState);
				sweepBuffer = new float[sweepBufferSize];
			}else{
				// store in continuous buffer
				if (buffer == NULL) {
					buffer = new float[fullBufferSize];
					std::fill(buffer, buffer + fullBufferSize, noDataValue);
				}else if(usedBufferSize > 0){
					std::fill(buffer, buffer + usedBufferSize, noDataValue);
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
				if(doCompress){
					std::fill(sweepBuffer, sweepBuffer + sweepBufferSize, noDataValue);
				}else{
					sweepBuffer = buffer + sweepOffset;
				}
				
				// fill in buffer from rays
				for (int theta = 0; theta < thetaSize; theta++) {
					Ray* ray = sweep->ray[theta];
					int maxDataDistance = 0;
					if (ray) {
						struct tm t = {0};  // Initalize to all 0's
						t.tm_year = ray->h.year - 1900; 
						t.tm_mon = ray->h.month - 1;
						t.tm_mday = ray->h.day;
						t.tm_hour = ray->h.hour;
						t.tm_min = ray->h.minute;
						t.tm_sec = ray->h.sec;
						#ifdef _WIN32
						time_t timeSinceEpoch = _mkgmtime(&t);
						#else
						time_t timeSinceEpoch = timegm(&t);
						#endif
						double rayDate = timeSinceEpoch + fmod(ray->h.sec, 1.0);
						//fprintf(stdout, "%f\n", rayDate);
						// exclude very inaccurate times, sometimes they are off by years
						if(lastRayDate == 0 || std::abs(lastRayDate - rayDate) < 10000){
							stats.beginTime = std::min(stats.beginTime, rayDate);
							stats.endTime = std::max(stats.endTime, rayDate);
							lastRayDate = rayDate;
						}else if(verbose){
							fprintf(stdout, "inaccurate date %f, last accepted is %f\n", rayDate, lastRayDate);
						}
						
						
						// get real angle of ray
						int realTheta = (int)((ray->h.azimuth * ((float)thetaBufferCount / 360.0f)) + thetaBufferCount) % thetaBufferCount;
						int radiusSize = std::min(ray->h.nbins, radiusBufferCount);
						RayInfo* thisRayInfo = &rayInfo[(thetaBufferCount + 2) * sweepIndex + (realTheta + 1)];
						thisRayInfo->actualAngle = ray->h.azimuth;
						thisRayInfo->interpolated = false;
						thisRayInfo->closestTheta = 0;
						for (int radius = 0; radius < radiusSize; radius++) {
							//int value = (ray->range[radius] - minValue) / divider;
							float value = ray->h.f(ray->range[radius]);
							//float value = ray->range[radius];
							// if (value == 131072) {
							// 	// value for no data
							// 	value = noDataValue;
							// }
							if (value == noDataValue){
								value = noDataValue + 0.000001;
							}
							if(value >= BADVAL - 10){
								value = noDataValue;
							}else{
								stats.minValue = value != 0 ? (value < stats.minValue ? value : stats.minValue) : stats.minValue;
								//minValue = value < minValue ? value : minValue;
								stats.maxValue = value > stats.maxValue ? value : stats.maxValue;
								maxDataDistance = std::max(maxDataDistance,radius);
							}
							if(value == RFVAL){
								//value = stats.invalidValue;
								value = noDataValue;
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
					float realMaxDistance = stats.innerDistance + maxDataDistance + 1;
					float realMaxHeight = realMaxDistance*std::sin(PIF / 180.0f * sweepInfo[sweepIndex].elevationAngle) + 1;
					stats.boundRadius = std::max(stats.boundRadius, realMaxDistance);
					stats.boundUpper = std::max(stats.boundUpper, realMaxHeight);
					stats.boundLower = std::min(stats.boundLower, realMaxHeight);
					//break;
				}
				
				
				InterpolateSweep(sweepIndex, sweepBuffer);
				
				
				if(doCompress){
					SparseCompress::compressValues(&compressorState, sweepBuffer, sweepBufferSize);
				}
				
				sweepIndex++;
			}
			
			usedBufferSize = sweepIndex * sweepBufferSize;
			
			if(doCompress){
				delete sweepBuffer;
				if(bufferCompressed){
					// remove old buffer
					delete[] bufferCompressed;
				}
				bufferCompressed = SparseCompress::compressEnd(&compressorState);
				compressedBufferSize = compressorState.sizeAllocated;
				if(verbose){
					fprintf(stderr, "Compressed size bytes:   %i\n", compressedBufferSize * 4);
					fprintf(stderr, "Uncompressed size bytes: %i\n", fullBufferSize * 4);
				}
			}
			

			/*
			fprintf(stderr,"ray values:");
			for(int radius = 0; radius < thetaBufferSize; radius++){
				fprintf(stderr," %.10f",buffer[radius + sweepBufferSize * 2]);
			}
			fprintf(stderr,"\n");
			//*/

			
		}else{
			return false;
		}
	}else{
		return false;
	}
	return true;
}

void RadarData::CopyFrom(RadarData* data) {
	if(buffer == NULL || thetaBufferCount != data->thetaBufferCount || sweepBufferCount != data->sweepBufferCount || radiusBufferCount != data->radiusBufferCount){
		//if (radiusBufferCount == 0) {
		radiusBufferCount = data->radiusBufferCount;
		//}
		//if (sweepBufferCount == 0) {
		if(sweepBufferCount != data->sweepBufferCount){
			if(sweepInfo != NULL){
				delete sweepInfo;
			}
			sweepInfo = new SweepInfo[data->sweepBufferCount]();
		}
		sweepBufferCount = data->sweepBufferCount;
		//}
		//if (thetaBufferCount == 0) {
		if(thetaBufferCount != data->thetaBufferCount || sweepBufferCount != data->sweepBufferCount){
			if(rayInfo != NULL){
				delete rayInfo;
			}
			rayInfo = new RayInfo[data->sweepBufferCount * (data->thetaBufferCount + 2)]();
		}
		thetaBufferCount = data->thetaBufferCount;
		//}
		
		thetaBufferSize = radiusBufferCount;
		sweepBufferSize = (thetaBufferCount + 2) * thetaBufferSize;
		fullBufferSize = sweepBufferCount * sweepBufferSize;
		
		
		// allocate buffer
		if(buffer != NULL){
			delete buffer;
		}
		buffer = new float[fullBufferSize];
		//std::fill(buffer, buffer+fullBufferSize, data->stats.noDataValue);
		if(data->buffer != buffer){
			// only set buffer used size when not copying from self
			usedBufferSize = 0;
		}
	}
	// copy data
	if(data->buffer != NULL && data->buffer != buffer){
		memcpy(buffer, data->buffer, std::min(fullBufferSize, data->usedBufferSize) * sizeof(float));
	}else if(data->bufferCompressed != NULL){
		SparseCompress::decompressToBuffer(buffer, data->bufferCompressed, fullBufferSize);
	}
	// take used buffer size into account
	if(data->usedBufferSize < usedBufferSize){
		// fill newly unused space
		std::fill(buffer + data->usedBufferSize, buffer + usedBufferSize, data->stats.noDataValue);
	}
	usedBufferSize = data->usedBufferSize;
	stats = data->stats;
	if(data->sweepInfo && data->sweepInfo != sweepInfo){
		memcpy(sweepInfo, data->sweepInfo, std::min(sweepBufferCount, data->sweepBufferCount) * sizeof(SweepInfo));
	}
	if(data->rayInfo && data->rayInfo != rayInfo){
		memcpy(rayInfo, data->rayInfo, std::min(sweepBufferCount * (thetaBufferCount + 2), data->sweepBufferCount * (data->thetaBufferCount + 2)) * sizeof(RayInfo));
	}
}





RadarData::TextureBuffer RadarData::CreateAngleIndexBuffer() {
	float* textureBuffer = new float[65536];
	std::fill(textureBuffer, textureBuffer + 65536, -1.0f);
	if(sweepInfo != NULL){
		//int divider = (stats.maxValue - stats.minValue) / 256 + 1;
		int firstIndex = sweepBufferCount;
		int lastIndex = -1;
		for (int sweepIndex = 0; sweepIndex < sweepBufferCount; sweepIndex++) {
			RadarData::SweepInfo &info = sweepInfo[sweepIndex];
			if(info.id != -1){
				firstIndex = std::min(firstIndex, sweepIndex);
				lastIndex = std::max(lastIndex, sweepIndex);
			}
			
		}
		for (int sweepIndex = firstIndex + 1; sweepIndex < sweepBufferCount; sweepIndex++) {
			RadarData::SweepInfo &info1 = sweepInfo[sweepIndex - 1];
			RadarData::SweepInfo &info2 = sweepInfo[sweepIndex];
			if(info2.id == -1){
				break;
			}
			int startLocation = info1.elevationAngle * 32768 / 90 + 32768;
			int endLocation = info2.elevationAngle * 32768 / 90 + 32768;
			int delta = endLocation - startLocation;
			float deltaF = delta;
			//fprintf(stderr, "delta: %i %f\n", delta, info1.elevation);
			float firstSweepIndex = sweepIndex - 1.0f;
			if (startLocation < 0 || endLocation >= 65536) {
				fprintf(stderr, "invalid elevation found while calculating angle index\n");
			} else {
				for (int i = 0; i <= delta; i++) {
					float subLocation = (float)i / deltaF;
					textureBuffer[startLocation + i] = firstSweepIndex + subLocation;
				}
			}
		}
		if(firstIndex == lastIndex){
			// display the single sweep with a width of 0.4 degrees
			RadarData::SweepInfo &info = sweepInfo[firstIndex];
			int startLocation = (info.elevationAngle - 0.2) * 32768 / 90 + 32768;
			int endLocation = (info.elevationAngle + 0.2) * 32768 / 90 + 32768;
			int delta = endLocation - startLocation;
			float firstSweepIndex = firstIndex;
			if (startLocation < 0 || endLocation >= 65536) {
				fprintf(stderr, "invalid elevation found while calculating angle index\n");
			} else {
				for (int i = 0; i <= delta; i++) {
					textureBuffer[startLocation + i] = firstSweepIndex;
				}
			}
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
	//return;
	/*if(bufferCompressed != NULL && buffer != NULL){
		delete bufferCompressed;
		bufferCompressed = NULL;
		compressedBufferSize = 0;
	}*/
	if(bufferCompressed == NULL && buffer != NULL){
		SparseCompress::CompressorState compressorState = {};
		compressorState.emptyValue = stats.noDataValue;
		compressorState.preCompressedSize = fullBufferSize / 10;
		SparseCompress::compressStart(&compressorState);
		SparseCompress::compressValues(&compressorState, buffer, usedBufferSize);
		bufferCompressed = SparseCompress::compressEnd(&compressorState);
		compressedBufferSize = compressorState.sizeAllocated;
	}
	if(buffer != NULL){
		delete buffer;
		buffer = NULL;
	}
}

void RadarData::Decompress() {
	if(bufferCompressed != NULL && buffer == NULL){
		CopyFrom(this);
	}
}

bool RadarData::IsCompressed() {
	return buffer == NULL && bufferCompressed != NULL;
}


void RadarData::Interpolate(){
	if(!buffer){
		fprintf(stderr, "Error: cannot interpolate without main buffer allocated\n");
		return;
	}
	for(int sweep = 0; sweep < sweepBufferCount; sweep++){
		InterpolateSweep(sweep, buffer + (sweep * sweepBufferSize));
	}
}

void RadarData::InterpolateSweep(int sweepIndex, float *sweepBuffer){
	// calculate RayInfo
	{
		// start of rays for this sweep
		int rayInfoOffset = (thetaBufferCount + 2) * sweepIndex + 1;
		int firstRay = -1;
		int lastRay = -1;
		RayInfo* lastRayInfo = NULL;
		for(int theta = 0; theta < thetaBufferCount; theta++){
			RayInfo* thisRayInfo = &rayInfo[rayInfoOffset + theta];
			if(!thisRayInfo->interpolated){
				if(firstRay == -1){
					firstRay = theta;
				}else{
					// fill in interpolated rays
					for(int i = lastRay + 1; i < theta; i++){
						RayInfo* interRayInfo = &rayInfo[rayInfoOffset + i];
						interRayInfo->previousTheta = lastRay - i;
						interRayInfo->nextTheta = theta - i;
						interRayInfo->theta = i;
						interRayInfo->sweep = sweepIndex;
						// angle of center interpolated of ray
						float angle = (i + 0.5) * 360.0f / (float)thetaBufferCount;
						if(thisRayInfo->actualAngle - angle < angle - lastRayInfo->actualAngle){
							interRayInfo->closestTheta = theta - i;
						}else{
							interRayInfo->closestTheta = lastRay - i;
						}
					}
					lastRayInfo->nextTheta = theta - lastRay;
					thisRayInfo->previousTheta = lastRay - theta;
					thisRayInfo->sweep = sweepIndex;
					thisRayInfo->theta = theta;
				}
				lastRay = theta;
				lastRayInfo = thisRayInfo;
			}
		}
		if(firstRay == -1){
			// no rays were in the sweep, just link first and last ray
			rayInfo[rayInfoOffset].previousTheta = thetaBufferCount - 1;
			rayInfo[rayInfoOffset + thetaBufferCount - 1].nextTheta = 1 - thetaBufferCount;
		}else{
			RayInfo* firstRayInfo = &rayInfo[rayInfoOffset + firstRay];
			// fill in warped around interpolated rays between last and first
			//fprintf(stderr, "%i %i   ", lastRay + 1, firstRay + thetaBufferCount);
			for(int i = lastRay + 1; i < firstRay + thetaBufferCount; i++){
				int iWrapped = i % thetaBufferCount;
				RayInfo* interRayInfo = &rayInfo[rayInfoOffset + iWrapped];
				interRayInfo->previousTheta = lastRay - iWrapped;
				interRayInfo->nextTheta = firstRay - iWrapped;
				interRayInfo->theta = iWrapped;
				interRayInfo->sweep = sweepIndex;
				// angle of center interpolated of ray
				float angle = (iWrapped + 0.5) * 360.0f / (float)thetaBufferCount;
				//fprintf(stderr, "%i %i %f %f %f   ", sweepIndex, iWrapped, (firstRayInfo->actualAngle + 360), angle, lastRayInfo->actualAngle);
				//if((firstRayInfo->actualAngle + 360) - angle < angle - (lastRayInfo->actualAngle)){
				if(moduloSmallerAbs(firstRayInfo->actualAngle - angle, lastRayInfo->actualAngle - angle, 360.0f)){
					interRayInfo->closestTheta = firstRay - iWrapped;
				}else{
					interRayInfo->closestTheta = lastRay - iWrapped;
				}
				//interRayInfo->closestTheta = -100;
			}
			// connect first and last ray
			lastRayInfo->nextTheta = firstRay - lastRay;
			lastRayInfo->theta = lastRay;
			lastRayInfo->sweep = sweepIndex;
			firstRayInfo->previousTheta = lastRay - firstRay;
			firstRayInfo->theta = firstRay;
			firstRayInfo->sweep = sweepIndex;
		}
		// create RayInfo for padded rays
		// I think these are broken so don't use them
		// They may actually work now but I haven't tested
		rayInfo[rayInfoOffset - 1] = rayInfo[rayInfoOffset + thetaBufferCount - 1];
		rayInfo[rayInfoOffset - 1].interpolated = true;
		rayInfo[rayInfoOffset - 1].closestTheta += thetaBufferCount;
		rayInfo[rayInfoOffset - 1].nextTheta += thetaBufferCount;
		rayInfo[rayInfoOffset - 1].previousTheta += thetaBufferCount;
		rayInfo[rayInfoOffset - 1].theta = -1;
		rayInfo[rayInfoOffset - 1].sweep = sweepIndex;
		rayInfo[rayInfoOffset + thetaBufferCount] = rayInfo[rayInfoOffset];
		rayInfo[rayInfoOffset + thetaBufferCount].interpolated = true;
		rayInfo[rayInfoOffset + thetaBufferCount].closestTheta -= thetaBufferCount;
		rayInfo[rayInfoOffset + thetaBufferCount].nextTheta -= thetaBufferCount;
		rayInfo[rayInfoOffset + thetaBufferCount].previousTheta -= thetaBufferCount;
		rayInfo[rayInfoOffset + thetaBufferCount].theta = thetaBufferCount;
		rayInfo[rayInfoOffset + thetaBufferCount].sweep = sweepIndex;
	}
	
	
	// interpolate ray data
	int rayInfoOffset = (thetaBufferCount + 2) * sweepIndex + 1;
	for (int theta = 0; theta < thetaBufferCount; theta++) {
		RayInfo* info = &rayInfo[rayInfoOffset + theta];
		if (info->interpolated) {
			// fill in blank by interpolating surroundings
			
			int gapDistance = info->nextTheta - info->previousTheta;
			if(gapDistance < 0){
				gapDistance += thetaBufferCount;
			}
			if(gapDistance <= std::max(thetaBufferCount / 50, 2) || true){
				int previousRayAbs = theta + info->previousTheta;
				int nextRayAbs = theta + info->nextTheta;
				if(nextRayAbs > thetaBufferCount || thetaBufferCount < 0 || previousRayAbs < 0 || previousRayAbs > thetaBufferCount){
					fprintf(stderr, "sgap distance %i %i %i %i %i %i\n", gapDistance, theta, previousRayAbs, nextRayAbs, info->previousTheta, info->nextTheta);
					continue;
				}
				for (int radius = 0; radius < radiusBufferCount; radius++) {
					float previousValue = sweepBuffer[radius + (previousRayAbs + 1) * thetaBufferSize];
					float nextValue = sweepBuffer[radius + (nextRayAbs + 1) * thetaBufferSize];
					for (int thetaTo = previousRayAbs + 1; thetaTo <= previousRayAbs + gapDistance - 1; thetaTo++) {
						float interLocation = (float)(thetaTo - previousRayAbs) / (float)(gapDistance);
						// write interpolated value
						sweepBuffer[radius + (modulo(thetaTo, thetaBufferCount) + 1) * thetaBufferSize] = previousValue * (1.0 - interLocation) + nextValue * interLocation;
					}
				}
				// advance past filled rays
				if(nextRayAbs >= theta){
					theta = nextRayAbs;
				}else{
					break;
				}
				
			}
		}
	}
	
	
	// pad theta with ray from other side to allow correct interpolation in shader at edges
	memcpy(
		sweepBuffer,
		sweepBuffer + (thetaBufferCount * thetaBufferSize),
		thetaBufferSize * sizeof(float));
	memcpy(
		sweepBuffer + (((thetaBufferCount + 1) * thetaBufferSize)),
		sweepBuffer + (thetaBufferSize),
		thetaBufferSize * sizeof(float));
}

int RadarData::MemoryUsage(){
	int usage = sizeof(RadarData);
	if(buffer != NULL){
		usage += sizeof(float) * fullBufferSize;
	}
	usage += sizeof(float) * compressedBufferSize;
	if(sweepInfo != NULL){
		usage += sizeof(SweepInfo) * sweepBufferCount;
	}
	if(rayInfo != NULL){
		usage += sizeof(RayInfo) * sweepBufferCount * (thetaBufferCount + 2);
	}
	return usage;
}

void RadarData::Deallocate(){
	if(buffer != NULL){
		delete[] buffer;
		buffer = NULL;
		usedBufferSize = 0;
	}
	if(bufferCompressed != NULL){
		delete[] bufferCompressed;
		bufferCompressed = NULL;
	}
	if(sweepInfo != NULL){
		delete[] sweepInfo;
		sweepInfo = NULL;
	}
	if(rayInfo != NULL){
		delete[] rayInfo;
		rayInfo = NULL;
	}
}



RadarData::~RadarData()
{
	RadarData::Deallocate();
	if(verbose){
		fprintf(stderr,"freed RadarData\n");
	}
}

void RadarData::TextureBuffer::Delete() {
	if(doDelete && data != NULL){
		delete[] data;
		data = NULL;
	}
}
