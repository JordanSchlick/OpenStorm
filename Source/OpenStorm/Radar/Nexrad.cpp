#include "Nexrad.h"
#include "Deps/rsl/wsr88d_decode_ar2v.h"
#include "Deps/zlib/zlib.h"
#include <fcntl.h>
#include <stdio.h>

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