#include "MiniRad.h"
#include "Deps/zlib/zlib.h"
#include "Deps/json11/json11.hpp"
#include "SystemAPI.h"
#include "SparseCompression.h"

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

#define PIF 3.14159265358979323846f

class MiniRadRadarReader::MiniRadInternal{
public:
	json11::Json jsonData;
	// directory of main file
	std::string directory;
	~MiniRadInternal(){
		
	}
};


bool MiniRadRadarReader::LoadFile(std::string filePath){
	verbose = true;
	internal = new MiniRadInternal;
	
	int lastSlash = std::max(std::max((int)filePath.find_last_of('/'), (int)filePath.find_last_of('\\')), -1);
	internal->directory = filePath.substr(0, lastSlash + 1);
	//std::string fileName = filePath.substr(lastSlash + 1);
	
	std::ifstream file(filePath);
	if(!file.is_open()){
		UnloadFile();
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadFile) could not open minirad file\n");
		return false;
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string err;
	internal->jsonData = json11::Json::parse(buffer.str(), err);
	
	if(!internal->jsonData.is_null()){
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadFile) minirad file has invalid json %s\n", err.c_str());
		// next if will not find the array and return
	}
	
	if(!internal->jsonData["volumes"].is_array()){
		UnloadFile();
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadFile) minirad file is missing volumes array\n");
		return false;
	}
	return true;
};

void MiniRadRadarReader::UnloadFile(){
	if(internal != NULL){
		delete internal;
		internal = NULL;
	}
}

MiniRadRadarReader::~MiniRadRadarReader(){
	UnloadFile();
}



class MiniRadRadarReader::RayData{
public:
	float angle = 0;
	size_t encodedSize = 0;
	uint8_t* encodedData = NULL;
	int encodeParamNonValueCount = 3;
	float* encodeParamNonValues;
	float encodeParamOffset = 0;
	float encodeParamScale = 1;
};
class MiniRadRadarReader::SweepData{
public:
	float elevation = 0;
	// int rayCount = 0;
	// int binCount = 0;
	float pixelSize = 0;
	float innerDistance = 0;
	std::vector<RayData> rays = {};
};

// reads a variable sized uint from a buffer with an offset of byteCounter and increments byteCounter by number of bytes used
inline uint64_t readVariableUint(const uint8_t* buffer, size_t* byteCounter){
	// move buffer to offset
	buffer += *byteCounter;
	uint64_t number = 0;
	size_t offset = 0;
	uint8_t byte;
	do{
		// get byte
		byte = *(buffer + offset);
		// add byte to number
		number |= (byte & 0b01111111) << (offset * 7);
		// move to next byte
		offset++;
		// check if the first bit is set to determine if to continue
	}while((byte & 0b10000000) == 0b10000000);
	*byteCounter += offset;
	return number;
}

bool MiniRadRadarReader::LoadVolume(RadarData* radarData, RadarData::VolumeType volumeType){
	
	double benchTime = SystemAPI::CurrentTime();
	
	json11::Json volumeJson;
	bool foundVolume = false;
	for(json11::Json volume : internal->jsonData["volumes"].array_items()){
		if(volume["volumeType"].is_number()){
			if(volumeType == volume["volumeType"].int_value()){
				volumeJson = volume;
				foundVolume = true;
			}
		}
	}
	if(!foundVolume){
		return false;
	}
	if(verbose){
		fprintf(stderr, "Minirad %s\n", volumeJson.dump().c_str());
	}
	std::string fileName = internal->directory + volumeJson["file"].string_value();
	SystemAPI::FileStats stats = SystemAPI::GetFileStats(fileName);
	if(!stats.exists || stats.isDirectory){
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadVolume) minirad data file does not exist\n");
		return false;
	}
	if(stats.size < 50){
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadVolume) minirad data file is empty\n");
		return false;
	}
	//sizeof(ShapeFileHeader)
	FILE* file = fopen(fileName.c_str(), "r");
	if(file == NULL){
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadVolume) could not open minirad data file\n");
		return false;
	}
	size_t fileSize = stats.size;
	// raw contents of the data file
	uint8_t* fileContents = new uint8_t[fileSize + 100];
	fread(fileContents, 1, fileSize, file);
	fclose(file);
	
	size_t location = 0;
	if(std::string((char*)fileContents, 11) != "MINIRADDATA"){
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadVolume) minirad data file is missing magic bytes\n");
		delete[] fileContents;
		return false;
	}
	location += 11;
	
	size_t jsonHeaderSize = readVariableUint(fileContents, &location);
	if(jsonHeaderSize > fileSize - location){
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadVolume) header overrun\n");
		delete[] fileContents;
		return false;
	}
	
	std::string headerString = std::string((char*)fileContents + location, jsonHeaderSize);
	std::string err;
	json11::Json header = json11::Json::parse(headerString, err);
	if(verbose){
		fprintf(stderr, "Minirad %i %s\n", (int)jsonHeaderSize, header.dump().c_str());
	}
	location += jsonHeaderSize;
	
	if(header["compression"] != "gzip"){
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadVolume) unsupported compression format\n");
		delete[] fileContents;
		return false;
	}
	// total allocated size of the buffer
	size_t decompressedBufferAllocated = fileSize;
	// amount of the buffer that has data
	size_t decompressedBufferSize = 0;
	if(volumeJson["fileSizeUncompressed"].is_number()){
		decompressedBufferAllocated = std::max(decompressedBufferAllocated, (size_t)volumeJson["fileSizeUncompressed"].number_value());
	}
	// buffer containing minirad messages
	uint8_t* decompressedBuffer = new uint8_t[decompressedBufferAllocated];
	if(decompressedBuffer == NULL){
		fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadVolume) decompressedBuffer allocation failure\n");
		delete[] fileContents;
		return false;
	}
	
	int zlibReturnStatus = Z_STREAM_END;
	// continue to decompress multiple streams until end of file or failure
	while(zlibReturnStatus == Z_STREAM_END){
		z_stream zStream;
		zStream.next_in   = fileContents + location;
		zStream.avail_in  = fileSize - location;
		zStream.total_in  = 0;
		zStream.next_out  = decompressedBuffer + decompressedBufferSize;
		zStream.avail_out = decompressedBufferAllocated - decompressedBufferSize;
		zStream.total_out = 0;
		zStream.zalloc    = Z_NULL;
		zStream.zfree     = Z_NULL;
		zStream.opaque    = Z_NULL;
		
		// 16+MAX_WBITS for gzip
    	zlibReturnStatus = inflateInit2(&zStream, 16+MAX_WBITS);
		while(zlibReturnStatus == Z_OK){
			// attempt to inflate the whole stream which should return Z_STREAM_END
			zlibReturnStatus = inflate(&zStream, Z_FINISH);
			if(zlibReturnStatus ==  Z_BUF_ERROR){
				// the output buffer could be reallocated here to continue decompression
				// if the minirad file has a proper fileSizeUncompressed this should not be nessasary
				// beware that Z_BUF_ERROR can also be caused by an uncompleted input stream
				// I think that out should be completely filled if it ran out of out buffer
				// next_out then needs to be shifted by total_out
				fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadVolume) warn: inflate ran out of buffer\n");
			}
		}
		if(zlibReturnStatus != Z_STREAM_END){
			fprintf(stderr, "MiniRad.cpp(MiniRadRadarReader::LoadVolume) warn: inflate returned error %i\n", zlibReturnStatus);
		}
		inflateEnd(&zStream);
		decompressedBufferSize += zStream.total_out;
		location += zStream.total_in;
		if(fileSize <= location){
			// whole file has been read
			break;
		}
	}
	// compressed file is no longer needed
	delete[] fileContents;
	
	if(verbose){
		fprintf(stderr, "Minirad decompressed from %i to %i\n", (int)fileSize, (int)decompressedBufferSize);
	}
	
	// location is now for the decompressed data
	location = 0;
	
	std::map<float, SweepData> sweepsMap = {};
	SweepData* currentSweep = NULL;
	size_t messageCount = 0;
	int   currentEncodeParamNonValueCount = 3;
	float currentEncodeParamNonValues[3] = {0,0,0};
	float currentEncodeParamOffset = 0;
	float currentEncodeParamScale = 1;
	// first pass of data to find information
	while(location < decompressedBufferSize){
		size_t messageType = readVariableUint(decompressedBuffer, &location);
		size_t messageLength = readVariableUint(decompressedBuffer, &location);
		size_t messageEnd = location + messageLength;
		switch(messageType){
			case 3: { // sweep
				// fprintf(stderr, "%i ", (int)messageType);
				float elevation = *(float*)(decompressedBuffer + location);
				location += 4;
				double startTime = *(double*)(decompressedBuffer + location);
				location += 8;
				double innerDistance = *(float*)(decompressedBuffer + location);
				location += 4;
				double pixelSize = *(float*)(decompressedBuffer + location);
				location += 4;
				if(sweepsMap.count(elevation) == 0){
					sweepsMap[elevation] = SweepData();
					currentSweep = &sweepsMap[elevation];
					currentSweep->elevation = elevation;
					currentSweep->innerDistance = innerDistance * pixelSize;
					currentSweep->pixelSize = pixelSize;
				}else{
					currentSweep = NULL;
				}
				// if(verbose){
				// 	fprintf(stderr, "Minirad sweep %f\n", elevation);
				// }
				break;
			}
			case 2: { // encoding params
				int encodingType = *(decompressedBuffer + location);
				location += 1;
				
				if(encodingType == 1){
					currentEncodeParamOffset = *(float*)(decompressedBuffer + location);
					location += 4;
					currentEncodeParamScale = *(float*)(decompressedBuffer + location);
					location += 4;
					currentEncodeParamNonValues[0] = *(float*)(decompressedBuffer + location);
					location += 4;
				}
				break;
			}
			case 4: { // ray
				if(currentSweep != NULL){
					int paramsSize = *(decompressedBuffer + location);
					location += 1;
					size_t dataStartLocation = location + paramsSize;
					RayData ray = {};
					ray.angle = *(float*)(decompressedBuffer + location);
					location += 4;
					ray.encodeParamNonValueCount = currentEncodeParamNonValueCount;
					ray.encodeParamNonValues = currentEncodeParamNonValues;
					ray.encodeParamOffset = currentEncodeParamOffset;
					ray.encodeParamScale = currentEncodeParamScale;
					if(dataStartLocation < messageEnd){
						ray.encodedSize = messageEnd - dataStartLocation;
						ray.encodedData = decompressedBuffer + dataStartLocation;
					}
					currentSweep->rays.push_back(ray);
				}
				break;
			}
		}
		
		location = messageEnd;
		messageCount++;
	}
	
	
	std::vector<SweepData> sweeps = {};
	//TODO: make this not arbitrary
	int maxRadius = 1000;
	int maxTheta = 0;
	float minPixelSize = INFINITY;
	
	for(auto sweep : sweepsMap){
		if(verbose){
			fprintf(stderr, "Minirad sweep ray count %i\n", (int)sweep.second.rays.size());
		}
		maxTheta = std::max(maxTheta, (int)sweep.second.rays.size());
		minPixelSize = std::min(minPixelSize, sweep.second.pixelSize);
		sweeps.push_back(sweep.second);
	}
	
	if(verbose){
		fprintf(stderr, "Minirad message count %i\n", (int)messageCount);
	}
	
	
	radarData->stats = RadarData::Stats();
	radarData->stats.latitude = internal->jsonData["latitude"].number_value();
	radarData->stats.longitude = internal->jsonData["longitude"].number_value();
	radarData->stats.altitude = internal->jsonData["altitude"].number_value();
	radarData->stats.volumeType = volumeType;
	
	
	radarData->stats.pixelSize = minPixelSize;
	// this could be a bad assumption if inner distances vary per sweep
	radarData->stats.innerDistance = sweeps[0].innerDistance / minPixelSize;
	if (radarData->sweepBufferCount == 0) {
		radarData->sweepBufferCount = sweeps.size();
	}
	if (radarData->radiusBufferCount == 0) {
		radarData->radiusBufferCount = maxRadius;
	}
	if (radarData->thetaBufferCount == 0) {
		radarData->thetaBufferCount = maxTheta;
	}
	radarData->thetaBufferSize = radarData->radiusBufferCount;
	radarData->sweepBufferSize = (radarData->thetaBufferCount + 2) * radarData->thetaBufferSize;
	radarData->fullBufferSize = radarData->sweepBufferCount * radarData->sweepBufferSize;
	if (radarData->rayInfo != NULL) {
		delete[] radarData->rayInfo;
	}
	radarData->rayInfo = new RadarData::RayInfo[radarData->sweepBufferCount * (radarData->thetaBufferCount + 2)]{};
	if (radarData->sweepInfo != NULL) {
		delete[] radarData->sweepInfo;
	}
	radarData->sweepInfo = new RadarData::SweepInfo[radarData->sweepBufferCount]{};
	
	// pointer to beginning of a sweep buffer to write to
	float* sweepBuffer = NULL;
	float noDataValue = radarData->stats.noDataValue;
	SparseCompress::CompressorState compressorState = {};
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
	for(int index = 0; index < sweeps.size(); index++){
		if (sweepIndex >= radarData->sweepBufferCount) {
			// ignore sweeps that wont fit in buffer
			break;
		}
		// const SweepData* sweep = &iterator->second;
		const SweepData* sweep = &sweeps[index];
		radarData->sweepInfo[sweepIndex].actualRayCount = sweep->rays.size();
		radarData->sweepInfo[sweepIndex].elevationAngle = sweep->elevation;
		radarData->sweepInfo[sweepIndex].id = sweepIndex;
		// fprintf(stderr, "%f %i %i %f %f\n", radarData->sweepInfo[sweepIndex].elevationAngle, sweep->rayCount, sweep->binCount, sweep->multiplier, sweep->offset);
		int thetaSize = sweep->rays.size();
		int sweepOffset = sweepIndex * radarData->sweepBufferSize;
		if(radarData->doCompress){
			std::fill(sweepBuffer, sweepBuffer + radarData->sweepBufferSize, noDataValue);
		}else{
			sweepBuffer = radarData->buffer + sweepOffset;
		}
		
		
		
		
		// fill in buffer from rays
		for (int theta = 0; theta < thetaSize; theta++){
			const MiniRadRadarReader::RayData *ray = &sweep->rays[theta];
			// get real angle of ray
			int realTheta = (int)((ray->angle * ((float)radarData->thetaBufferCount / 360.0f)) + radarData->thetaBufferCount) % radarData->thetaBufferCount;
			
			RadarData::RayInfo* thisRayInfo = &radarData->rayInfo[(radarData->thetaBufferCount + 2) * sweepIndex + (realTheta + 1)];
			thisRayInfo->actualAngle = ray->angle;
			thisRayInfo->interpolated = false;
			thisRayInfo->closestTheta = 0;
			// move everything important into local variables to allow optimization
			int encodeParamNonValueCount = ray->encodeParamNonValueCount;
			float* encodeParamNonValues = ray->encodeParamNonValues;
			int thetaIndex = (realTheta + 1) * radarData->thetaBufferSize;
			float multiplier = ray->encodeParamScale;
			// take encodeParamNonValueCount in offset to avoid doing it in the loop
			float offset = ray->encodeParamOffset + encodeParamNonValueCount * multiplier;
			uint8_t* rayEncodedBuffer = ray->encodedData;
			uint8_t* rayEncodedBufferEnd = ray->encodedData + ray->encodedSize;
			// scale of ray vs output buffer
			float scale = minPixelSize / sweep->pixelSize;
			int radiusSize = radarData->radiusBufferCount;
			// fprintf(stderr, "%i %i \n",thetaSize, radiusSize);
			int inputIndex = -1;
			uint16_t runningValue = 0;
			int radius;
			for (radius = 0; radius < radiusSize; radius++) {
				//int value = (ray->range[radius] - minValue) / divider;
				int newInputIndex = (int)(radius * scale);
				if(inputIndex == newInputIndex){
					// have not moved on to new index so use old value and don't advance
					sweepBuffer[thetaIndex + radius] = sweepBuffer[thetaIndex + radius - 1];
				}else{
					uint8_t firstByte = *rayEncodedBuffer;
					rayEncodedBuffer += 1;
					if((firstByte & 0b10000000) == 0b10000000){
						uint8_t secondByte = *rayEncodedBuffer;
						rayEncodedBuffer += 1;
						// set running value from bits from both bytes
						runningValue = (((uint16_t)(firstByte & 0b1111111)) << 8) | secondByte;
					}else{
						// sign extend the 7 bit number to 8 bits
						firstByte = firstByte | ((firstByte & 0b1000000) << 1);
						// interpret as signed int and add to runningValue
						runningValue += *(int8_t*)(&firstByte);
						//runningValue = 0;
					}
					
					if(runningValue < encodeParamNonValueCount){
						sweepBuffer[thetaIndex + radius] = encodeParamNonValues[runningValue];
					}else{
						float value = runningValue * multiplier + offset;
						if (value == noDataValue){
							// prevent real data from having the same value as invalid data
							value = noDataValue + 0.000001;
						}
						minValue = value != 0 ? (value < minValue ? value : minValue) : minValue;
						maxValue = value > maxValue ? value : maxValue;
						sweepBuffer[thetaIndex + radius] = value;
					}
					
					if(rayEncodedBuffer >= rayEncodedBufferEnd){
						// reached end of encoded buffer
						break;
					}
				}
			}
			
			
			float realMaxDistance = radarData->stats.innerDistance + radius + 1;
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
	if(verbose){
		fprintf(stderr, "volume loading code took %fs\n", benchTime);
	}
	
	
	delete[] decompressedBuffer;
	return true;
}