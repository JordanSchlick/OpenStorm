#include "SparseCompression.h"

#include <stdio.h>
#include <algorithm>

namespace SparseCompress{
	
	void compressPreRunAddEmptySpace(CompressorState* state, int emptySize) {
		if(state->preWasEmpty == false){
			state->preCompressedSize += 2;
			state->preWasEmpty = true;
		}
		state->preUncompressedSize += emptySize;
	}
	
	
	
	// allocates buffer
	void compressStart(CompressorState* state){
		if(state->preCompressedSize <= 5){
			state->preCompressedSize = 16384;
		}
		state->sizeAllocated = state->preCompressedSize;
		state->bufferFloat = new float[state->sizeAllocated];
		state->bufferInt = (uint32_t*)state->bufferFloat;
		state->bufferFloat[0] = state->emptyValue;
		//fprintf(stderr, "Compressed size bytes:   %i\n", state->preCompressedSize * 4);
		//fprintf(stderr, "Uncompressed size bytes: %i\n", state->preUncompressedSize * 4);
	}
	
	void compressValues(CompressorState* state, float* values, int number){
		float emptyValue = state->emptyValue;
		for(int i = 0; i < number; i++){
			float value = values[i];
			if(state->sizeAllocated < state->size + 2){
				state->sizeAllocated *= state->doShrink ? 2 : 1.5;
				float* newBuffer = new float[state->sizeAllocated];
				memcpy(newBuffer, state->bufferFloat, state->size * 4);
				delete[] state->bufferFloat;
				state->bufferFloat = newBuffer;
				state->bufferInt = (uint32_t*)newBuffer;
			}
			if(value == emptyValue){
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

	void compressAddEmptySpace(CompressorState* state, int emptySize) {
		if(state->wasEmpty == false){
			// store data amount when blank space begins
			state->bufferInt[state->sizeInfoLocation + 1] = state->deltaLocation;
			state->deltaLocation = 0;
			state->wasEmpty = true;
			// create space for sizeInfo
			state->sizeInfoLocation = state->size;
			state->size += 2;
		}
		state->deltaLocation += emptySize;
	}

	// end compression
	float* compressEnd(CompressorState* state){
		// add size for end info
		state->size += 2;
		if((state->doShrink && state->size < state->sizeAllocated) || state->size > state->sizeAllocated){
			// reallocate to final size
			state->sizeAllocated = state->size;
			float* newBuffer = new float[state->sizeAllocated];
			memcpy(newBuffer, state->bufferFloat, state->size * 4);
			delete[] state->bufferFloat;
			state->bufferFloat = newBuffer;
			state->bufferInt = (uint32_t*)newBuffer;
		}
		if(state->wasEmpty == false){
			// store data amount
			state->bufferInt[state->sizeInfoLocation + 1] = state->deltaLocation;
		}else{
			// store blank space amount
			state->bufferInt[state->sizeInfoLocation] = state->deltaLocation;
			state->bufferInt[state->sizeInfoLocation + 1] = 0;
		}
		// end info
		state->bufferInt[state->size - 2] = 0;
		state->bufferInt[state->size - 1] = 0;
		
		return state->bufferFloat;
	}

	int decompressToBuffer(float* destinationBuffer, float* sourceCompressedBuffer, int maxSize) {
		uint32_t* intBuf = (uint32_t*)sourceCompressedBuffer;
		float emptyValue = sourceCompressedBuffer[0];
		int decompressedSize = 0;
		int location = 1;
		while(decompressedSize < maxSize){
			int blankSize = intBuf[location];
			int dataSize = intBuf[location + 1];
			if(blankSize == 0 && dataSize == 0){
				//reached end
				break;
			}
			// fill destination with zeros
			int blankSizeDo = std::min(maxSize - decompressedSize, blankSize);
			std::fill(destinationBuffer + decompressedSize, destinationBuffer + decompressedSize + blankSizeDo, emptyValue);
			decompressedSize += blankSizeDo;
			if(blankSizeDo < blankSize){
				// destination is full
				break;
			}
			// copy to destination
			int dataSizeDo = std::min(maxSize - decompressedSize, dataSize);
			memcpy(destinationBuffer + decompressedSize, sourceCompressedBuffer + location + 2, dataSizeDo * 4);
			decompressedSize += dataSizeDo;
			if(dataSizeDo < dataSize){
				// destination is full
				break;
			}
			
			// advance
			location += 2 + dataSize;
		}
		
		return decompressedSize;
	}
}