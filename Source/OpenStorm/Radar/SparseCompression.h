#pragma once

#include <stdint.h>
#include <string.h>
#include <math.h>


namespace SparseCompress{
	
	/*
	Sparse compression format
	This format drops out blank space in the buffer and stores data contiguously.
	Seeking is not supported because no index is created. It must be decompressed before use.
	The first value contained in the compressed buffer is the value that was contained in the blank space.
	Blank space is compressed down into two ints. The format is [uint32 blankSize,uint32 dataSize]
	blankSize specifies the amount of blank data that was removed at that spot in the array.
	dataSize specifies how much data follows in the compressed array until the next two ints.
	The next blank space can be found at the end of the data size specifed.
	The last blank space info has a size of zero for both values.
	
	example of compressed buffer:
	[
		float blank space value, 
		uint32 blankSize, uint32 dataSize, float data, ..., float data,
		uint32 blankSize, uint32 dataSize, float data, ..., float data,
		uint32 blankSize, uint32 dataSize, float data, ..., float data,
		...,
		0, 0
	]
	*/
	class CompressorState{
	public:
		int sizeInfoLocation = 1;
		int deltaLocation = 0;
		int size = 3;
		int sizeAllocated = 0;
		// value to compress as empty space
		float emptyValue = -INFINITY;
		uint32_t* bufferInt;
		// compressed buffer
		float* bufferFloat;
		bool wasEmpty = true;
		
		// variables for prerun
		// prerun can be used to find the exact size of the data and allocate buffer accordingly
		
		// preCompressedSize can be manually set for an initial buffer size
		int preCompressedSize = 5;
		int preUncompressedSize = 0;
		bool preWasEmpty = true;
		
		// set to true if the buffer should be resized to perfectly fit at the end
		bool doShrink = true;
	};
	
	// calculates size of buffer based on values this is run on
	// pre run is not necessary but will prevent reallocations by determining the exact buffer size ahead of time
	inline void compressPreRun(CompressorState* state, float value);
	
	// calculates size of buffer based on values this is run on
	// pre run is not necessary but will prevent reallocations by determining the exact buffer size ahead of time
	inline void compressPreRunAddEmptySpace(CompressorState* state, int emptySize);
	
	// allocates buffer
	void compressStart(CompressorState* state);
	
	// add a single float to the compressed buffer
	inline void compressValue(CompressorState* state, float value);
	
	// add a region of empty space
	void compressAddEmptySpace(CompressorState* state, int emptySize);
	
	// add a buffer to compress
	void compressValues(CompressorState* state, float* values, int number);
	
	// end compression and return buffer
	float* compressEnd(CompressorState* state);
	
	// decompress to destinationBuffer of maxSize in size
	// returns size of decompressed data
	int decompressToBuffer(float* destinationBuffer, float* sourceCompressedBuffer, int maxSize);
	
	
	
	
	// define inline funtions
	
	// calculates size of buffer based on values this is run on
	inline void compressPreRun(CompressorState* state, float value){
		if(value == -INFINITY){
			if(state->preWasEmpty == false){
				state->preCompressedSize += 2;
				state->preWasEmpty = true;
			}
		}else{
			state->preCompressedSize++;
			state->preWasEmpty = false;
		}
		state->preUncompressedSize++;
	}
	
	// add a single float to the compressed buffer
	inline void compressValue(CompressorState* state, float value){
		if(state->sizeAllocated == state->size){
			state->sizeAllocated *= state->doShrink ? 2 : 1.5;
			float* newBuffer = new float[state->sizeAllocated];
			memcpy(newBuffer, state->bufferFloat, state->size * 4);
			delete[] state->bufferFloat;
			state->bufferFloat = newBuffer;
			state->bufferInt = (uint32_t*)newBuffer;
		}
		if(value == -INFINITY){
			if(state->wasEmpty == false){
				// store data amount when blank space begins
				state->bufferInt[state->sizeInfoLocation + 1] = state->deltaLocation;
				state->deltaLocation = 0;
				state->wasEmpty = true;
				// create space for sizeInfo
				state->sizeInfoLocation = state->size;
				state->size += 2;
			}
		}else{
			if(state->wasEmpty == true){
				// store blank space amount when data begins
				state->bufferInt[state->sizeInfoLocation] = state->deltaLocation;
				state->deltaLocation = 0;
				state->wasEmpty = false;
			}
			state->bufferFloat[state->size] = value;
			state->size++;
		}
		state->deltaLocation++;
	}
}