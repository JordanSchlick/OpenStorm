#include "MiniRad.h"
#include "Deps/zlib/zlib.h"
#include "Deps/json11/json11.hpp"
#include "SystemAPI.h"

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>


class MiniRadRadarReader::MiniRadInternal{
public:
	json11::Json jsonData;
	// directory of main file
	std::string directory;
	~MiniRadInternal(){
		
	}
};


bool MiniRadRadarReader::LoadFile(std::string filePath){
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
	fprintf(stderr, "TEST TEST %s\n", volumeJson.dump().c_str());
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
	fprintf(stderr, "TEST TEST %i %s\n", (int)jsonHeaderSize, headerString.c_str());
	// json header = json::parse(headerString);
	std::string err;
	json11::Json header = json11::Json::parse(headerString, err);
	fprintf(stderr, "TEST TEST %i %s\n", (int)jsonHeaderSize, header.dump().c_str());
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
	
	fprintf(stderr, "TEST TEST Minirad decompressed from %i to %i\n", (int)fileSize, (int)decompressedBufferSize);
	
	// location is now for the decompressed data
	location = 0;
	size_t messageCount = 0;
	// first pass of data to find information
	while(location < decompressedBufferSize){
		size_t messageType = readVariableUint(decompressedBuffer, &location);
		size_t messageLength = readVariableUint(decompressedBuffer, &location);
		size_t messageEnd = location + messageLength;
		
		
		
		
		location = messageEnd;
		messageCount++;
	}
	fprintf(stderr, "TEST TEST Minirad message count %i\n", (int)messageCount);
	
	delete[] decompressedBuffer;
	return false;
}