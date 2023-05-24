#include "Tar.h"

#define BLOCKSIZE 512

typedef struct TarHeader{
	char name[100];
	char mode[8];
	char owner[8];
	char group[8];
	char size[12];
	char modificationDate[12];
	char checksum[8];
	char linkIndicator[1];
	char linkName[100];
} TarHeader;


Tar::Tar(std::string filename){
	tarFileName = filename;
	FILE* file = fopen(tarFileName.c_str(), "r");
	if(file == NULL){
		valid = false;
		return;
	}
	uint8_t block[BLOCKSIZE];
	size_t offset = 0;
	//fprintf(stderr, "Begin tar parse\n");
	while(fread(block, 1, BLOCKSIZE, file) == BLOCKSIZE){
		offset += BLOCKSIZE;
		//fprintf(stderr, "read ");
		if(block[0] != 0){
			TarHeader* header = (TarHeader*)block;
			TarFile tarFile = {};
			header->name[99] = 0;
			tarFile.name = std::string(header->name);
			tarFile.offset = offset;
			int index = 0;
			while(index < 12 && header->size[index] != 0){
				tarFile.size *= 8;
				tarFile.size += header->size[index] - '0';
				index++;
			}
			size_t seekAmount = tarFile.size;
			size_t seekAmountExtra = seekAmount % BLOCKSIZE;
			if(seekAmountExtra > 0){
				seekAmount += BLOCKSIZE - seekAmountExtra;
			}
			fseek(file, seekAmount, SEEK_CUR);
			offset += seekAmount;
			//fprintf(stderr, "%s %i\n", tarFile.name.c_str(), (int)tarFile.size);
			fileMap[tarFile.name] = tarFile;
		}
	}
	fclose(file);
	valid = true;
	//fprintf(stderr, "Finish tar parse\n");
}

Tar::TarFile* Tar::GetFile(std::string filename){
	if(fileMap.find(filename) == fileMap.end()){
		return NULL;
	}
	FILE* file = fopen(tarFileName.c_str(), "r");
	if(file == NULL){
		return NULL;
	}
	TarFile* tarFile = new TarFile();
	*tarFile = fileMap[filename];
	tarFile->data = new uint8_t[tarFile->size];
	fseek(file, tarFile->offset, SEEK_SET);
	fread(tarFile->data, 1, tarFile->size, file);
	fclose(file);
	return tarFile;
}