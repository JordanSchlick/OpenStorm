#include "Nexrad.h"
#include "Deps/rsl/rsl.h"
#include "Deps/rsl/wsr88d_decode_ar2v.h"
#include "Deps/zlib/zlib.h"
#include "NexradSites/NexradSites.h"
#include "SparseCompression.h"
#include "SystemAPI.h"
#include <fcntl.h>
#include <stdio.h>
#include <cstring>
#include <map>
#include <algorithm>
#include <ctime>
#include <cmath>

#define PIF 3.14159265358979323846f


int Nexrad::RecompressArchive(std::string inFileName, std::string outFileName){
	#ifdef _WIN32
		_set_fmode(_O_BINARY);
	#endif // _WIN32
	
	FILE* inFile = fopen(inFileName.c_str(), "r");
	if (inFile == NULL){
		fprintf(stderr,"failed open file for recompression\n");
		return -1;
	}
	
	// parse nexrad file for compression
	unsigned char hdrplus4[28];
	char bzmagic[4];
	if (fread(hdrplus4, sizeof(hdrplus4), 1, inFile) != 1) {
		fprintf(stderr,"failed to read first 28 bytes of Wsr88d file\n");
		fclose(inFile);
		return -1;
	}
	if (fread(bzmagic, sizeof(bzmagic), 1, inFile) != 1) {
		fprintf(stderr,"failed to read bzip magic bytes from Wsr88d file\n");
		fclose(inFile);
		return -1;
	}
	bool isGzip = false;
	bool isBzip = false;
	// test for bzip2 magic.
	if (strncmp("BZ", bzmagic,2) == 0){
		isBzip = true;
	}
	// test for gzip magic bytes
	if (hdrplus4[0] == 0x1f && hdrplus4[1] == 0x8b){
		isGzip = true;
	}
	fclose(inFile);
	// reopen the file
	inFile = fopen(inFileName.c_str(), "r");
	// decompress
	if(isGzip){
		inFile = uncompress_pipe_gzip(inFile);
	} else if(isBzip){
		inFile = uncompress_pipe_ar2v(inFile);
	}
	
	gzFile outGzFile = gzopen(outFileName.c_str(), "wb");
	int chunkSize = 65536;
	uint8_t* buffer = new uint8_t[chunkSize];
	int readAmount = chunkSize;
	while(readAmount == chunkSize){
		readAmount = fread(buffer, 1, chunkSize, inFile);
		gzwrite(outGzFile, buffer, readAmount);
	}
	delete[] buffer;
	gzclose(outGzFile);
	fclose(inFile);
	return 0;
}
bool NexradRadarReader::LoadFile(std::string filename) {
	UnloadFile();
	RSL_wsr88d_keep_sails();
	Radar* radar = RSL_wsr88d_to_radar((char*)filename.c_str(), (char*)"");

	//UE_LOG(LogTemp, Display, TEXT("================ %i"), radar);
	
	if(verbose && radar){
		for(int i = 0; i <= 46; i++){
			Volume* volume = radar->v[i];
			if(volume){
				fprintf(stderr, "Volume %i %s\n", i, volume->h.type_str);
			}
		}
	}
	rslData = radar;
	return radar != NULL;
}

bool NexradRadarReader::LoadVolume(RadarData *radarData, RadarData::VolumeType volumeType) {
	double benchTime = SystemAPI::CurrentTime();
	Radar* radar = (Radar*)rslData;
	if (radar == NULL) {
		return false;
	}
	radarData->stats = RadarData::Stats();
	int nexradType = DZ_INDEX;
	switch (volumeType){
		case RadarData::VOLUME_REFLECTIVITY:
			nexradType = DZ_INDEX;
			break;
		case RadarData::VOLUME_VELOCITY:
			nexradType = VR_INDEX;
			radarData->stats.noDataValue = 0;
			break;
		case RadarData::VOLUME_SPECTRUM_WIDTH:
			nexradType = SW_INDEX;
			break;
		case RadarData::VOLUME_CORELATION_COEFFICIENT:
			nexradType = RH_INDEX;
			break;
		case RadarData::VOLUME_DIFFERENTIAL_REFLECTIVITY:
			nexradType = DR_INDEX;
			break;
		case RadarData::VOLUME_DIFFERENTIAL_PHASE_SHIFT:
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
		radarData->stats.latitude = site->latitude;
		radarData->stats.longitude = site->longitude;
		radarData->stats.altitude = site->altitude;
	}
	if (volume == NULL) {
		return false;
	}
	radarData->stats.volumeType = volumeType;
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



	
	if (radarData->sweepBufferCount == 0) {
		radarData->sweepBufferCount = sweeps.size();
	}
	if (radarData->sweepInfo != NULL) {
		delete[] radarData->sweepInfo;
	}
	radarData->sweepInfo = new RadarData::SweepInfo[radarData->sweepBufferCount]{};
	int sweepId = 0;
	int maxTheta = 0;
	int maxRadius = 0;
	radarData->stats.minValue = INFINITY;
	radarData->stats.maxValue = -INFINITY;
	radarData->stats.boundUpper = 0;
	radarData->stats.boundLower = 1e-10;
	radarData->stats.boundRadius = 0;
	radarData->stats.beginTime = INFINITY;
	radarData->stats.endTime = -INFINITY;
	double lastRayDate = 0;
	
	// find info about sweeps
	for (const auto pair : sweeps) {
		if (sweepId >= radarData->sweepBufferCount) {
			// ignore sweeps that wont fit in buffer
			break;
		}
		Sweep* sweep = pair.second;
		radarData->sweepInfo[sweepId].id = sweep->h.sweep_num;
		radarData->sweepInfo[sweepId].index = sweepId;
		radarData->sweepInfo[sweepId].elevationAngle = sweep->h.elev;
		radarData->sweepInfo[sweepId].actualRayCount = sweep->h.nrays;
		radarData->stats.innerDistance = (float)sweep->ray[0]->h.range_bin1 / (float)sweep->ray[0]->h.gate_size;
		radarData->stats.pixelSize = (float)sweep->ray[0]->h.gate_size;
		//fprintf(stderr, "innerDistance: %f\n", innerDistance);

		int thetaSize = sweep->h.nrays;
		maxTheta = std::max(maxTheta, thetaSize);
		
		for (int theta = 0; theta < thetaSize; theta++) {
			Ray* ray = sweep->ray[theta];
			if(ray){
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
					radarData->stats.beginTime = std::min(radarData->stats.beginTime, rayDate);
					radarData->stats.endTime = std::max(radarData->stats.endTime, rayDate);
					lastRayDate = rayDate;
				}else if(verbose){
					fprintf(stdout, "inaccurate date %f, last accepted is %f\n", rayDate, lastRayDate);
				}
				
				maxRadius = std::max(maxRadius, ray->h.nbins);
			}
		}
		
		sweepId++;
	}
	
	//fprintf(stderr, "bounds %f %f %f\n",stats.boundRadius,stats.boundUpper,stats.boundLower);
	
	
	if (radarData->stats.beginTime == INFINITY) {
		radarData->stats.beginTime = 0;
		radarData->stats.endTime = 0;
	}
	if (radarData->radiusBufferCount == 0) {
		radarData->radiusBufferCount = maxRadius;
	}
	if (radarData->thetaBufferCount == 0) {
		radarData->thetaBufferCount = maxTheta;
	}

	if(verbose){
		fprintf(stderr, "min: %f   max: %f\n", radarData->stats.minValue, radarData->stats.maxValue);
	}
	
	// sizes of sections of the buffer
	radarData->thetaBufferSize = radarData->radiusBufferCount;
	radarData->sweepBufferSize = (radarData->thetaBufferCount + 2) * radarData->thetaBufferSize;
	radarData->fullBufferSize = radarData->sweepBufferCount * radarData->sweepBufferSize;
	
	if (radarData->rayInfo != NULL) {
		delete[] radarData->rayInfo;
	}
	radarData->rayInfo = new RadarData::RayInfo[radarData->sweepBufferCount * (radarData->thetaBufferCount + 2)]{};
	
	float noDataValue = radarData->stats.noDataValue;
	
	SparseCompress::CompressorState compressorState = {};
	
	// pointer to beginning of a sweep buffer to write to
	float* sweepBuffer = NULL;
	
	if(radarData->doCompress){
		// store in compressed form
		compressorState.preCompressedSize = radarData->fullBufferSize / 10;
		compressorState.emptyValue = radarData->stats.noDataValue;
		SparseCompress::compressStart(&compressorState);
		sweepBuffer = new float[radarData->sweepBufferSize];
	}else{
		// store in continuous buffer
		if (radarData->buffer == NULL) {
			radarData->buffer = new float[radarData->fullBufferSize];
			std::fill(radarData->buffer, radarData->buffer + radarData->fullBufferSize, noDataValue);
		}else if(radarData->usedBufferSize > 0){
			std::fill(radarData->buffer, radarData->buffer + radarData->usedBufferSize, noDataValue);
		}
	}
	
	float minValue = INFINITY;
	float maxValue = -INFINITY;
	int sweepIndex = 0;
	for (const auto pair : sweeps) {
		if (sweepIndex >= radarData->sweepBufferCount) {
			// ignore sweeps that wont fit in buffer
			break;
		}
		Sweep* sweep = pair.second;
		int thetaSize = sweep->h.nrays;
		int sweepOffset = sweepIndex * radarData->sweepBufferSize;
		if(radarData->doCompress){
			std::fill(sweepBuffer, sweepBuffer + radarData->sweepBufferSize, noDataValue);
		}else{
			sweepBuffer = radarData->buffer + sweepOffset;
		}
		
		
		
		// fill in buffer from rays
		for (int theta = 0; theta < thetaSize; theta++) {
			Ray* ray = sweep->ray[theta];
			int maxDataDistance = 0;
			if (ray) {
				
				// get real angle of ray
				int realTheta = (int)((ray->h.azimuth * ((float)radarData->thetaBufferCount / 360.0f)) + radarData->thetaBufferCount) % radarData->thetaBufferCount;
				int radiusSize = std::min(ray->h.nbins, radarData->radiusBufferCount);
				RadarData::RayInfo* thisRayInfo = &radarData->rayInfo[(radarData->thetaBufferCount + 2) * sweepIndex + (realTheta + 1)];
				thisRayInfo->actualAngle = ray->h.azimuth;
				thisRayInfo->interpolated = false;
				thisRayInfo->closestTheta = 0;
				int thetaIndex = (realTheta + 1) * radarData->thetaBufferSize;
				for (int radius = 0; radius < radiusSize; radius++) {
					//int value = (ray->range[radius] - minValue) / divider;
					float value = ray->h.f(ray->range[radius]);
					//float value = ray->range[radius];
					// if (value == 131072) {
					// 	// value for no data
					// 	value = noDataValue;
					// }
					if (value == noDataValue){
						// prevent real data from having the same value as invalid data
						value = noDataValue + 0.000001;
					}
					if(value >= BADVAL - 10){
						value = noDataValue;
					}else{
						minValue = value != 0 ? (value < minValue ? value : minValue) : minValue;
						maxValue = value > maxValue ? value : maxValue;
						maxDataDistance = std::max(maxDataDistance,radius);
					}
					if(value == RFVAL){
						//value = stats.invalidValue;
						value = noDataValue;
					}
					sweepBuffer[thetaIndex + radius] = value;

					//if (theta == 0) {
					//	value = 255;
					//}
					//RawImageData[3 + radius * 4 + realTheta * radiusBufferSize + sweepIndex * thetaRadiusBufferSize] = std::max(0, value);
					//RawImageData[3 + radius * 4 + theta * radiusSize * 4] = 0;
					//setBytes++;
				}
			}
			float realMaxDistance = radarData->stats.innerDistance + maxDataDistance + 1;
			float realMaxHeight = realMaxDistance*std::sin(PIF / 180.0f * radarData->sweepInfo[sweepIndex].elevationAngle) + 1;
			radarData->stats.boundRadius = std::max(radarData->stats.boundRadius, realMaxDistance);
			radarData->stats.boundUpper = std::max(radarData->stats.boundUpper, realMaxHeight);
			radarData->stats.boundLower = std::min(radarData->stats.boundLower, realMaxHeight);
			//break;
		}
		
		
		radarData->InterpolateSweep(sweepIndex, sweepBuffer);
		
		
		if(radarData->doCompress){
			SparseCompress::compressValues(&compressorState, sweepBuffer, radarData->sweepBufferSize);
		}
		
		sweepIndex++;
	}
	if (minValue == INFINITY) {
		minValue = 0;
		maxValue = 1;
	}
	radarData->stats.minValue = minValue;
	radarData->stats.maxValue = maxValue;
	
	radarData->usedBufferSize = sweepIndex * radarData->sweepBufferSize;
	
	if(radarData->doCompress){
		delete sweepBuffer;
		if(radarData->bufferCompressed){
			// remove old buffer
			delete[] radarData->bufferCompressed;
		}
		radarData->bufferCompressed = SparseCompress::compressEnd(&compressorState);
		radarData->compressedBufferSize = compressorState.sizeAllocated;
		if(verbose){
			fprintf(stderr, "Compressed size bytes:   %i\n", radarData->compressedBufferSize * 4);
			fprintf(stderr, "Uncompressed size bytes: %i\n", radarData->fullBufferSize * 4);
		}
	}
	
	benchTime = SystemAPI::CurrentTime() - benchTime;
	if(verbose || true){
		fprintf(stderr, "volume loading code took %fs\n", benchTime);
	}

	/*
	fprintf(stderr,"ray values:");
	for(int radius = 0; radius < thetaBufferSize; radius++){
		fprintf(stderr," %.10f",buffer[radius + sweepBufferSize * 2]);
	}
	fprintf(stderr,"\n");
	//*/
	return true;
}

void NexradRadarReader::UnloadFile() {
	if(rslData != NULL){
		RSL_free_radar((Radar*)rslData);
		rslData = NULL;
	}
}

NexradRadarReader::~NexradRadarReader(){
	UnloadFile();
}
