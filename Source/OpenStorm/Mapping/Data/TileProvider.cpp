#include "TileProvider.h"
#include "../../Radar/SystemAPI.h"
#include <cmath>
#include <stdio.h>

void Tile::SetCallback(std::function<void()> callback){
	if(isReady){
		callback();
	}else{
		readyCallback = callback;
	}
}

Tile::~Tile(){
	if(data != NULL){
		delete[] data;
	}
}

TileProvider::TileProvider(std::string name, std::string url, std::string imageType, int maxZoom) {
	this->name = name;
	this->url = url;
	this->imageType = imageType;
	this->maxZoom = maxZoom;
}

Tile *TileProvider::GetTile(int zoom, int y, int x){
	Tile* tile = new Tile();
	tile->tileProvider = this;
	if(zoom > maxZoom){
		return tile;
	}
	int numSize = std::to_string((int)pow(2, zoom)).length();
	std::string nameX = std::to_string(x);
	nameX = std::string(numSize - nameX.length(), '0') + nameX;
	std::string nameY = std::to_string(y);
	nameY = std::string(numSize - nameY.length(), '0') + nameY;
	std::string nameZ = std::to_string(zoom);
	nameZ = std::string(2 - nameZ.length(), '0') + nameZ;
	std::string extension;
	if(imageType == "image/jpeg"){
		extension = ".jpg";
	}else if(imageType == "image/png"){
		extension = ".png";
	}
	tile->fileName = "z" + nameZ + "y" + nameY + "x" + nameX + extension;
	if(staticCache != ""){
		std::string path = staticCache + tile->fileName;
		//fprintf(stderr, "%s", path.c_str());
		SystemAPI::FileStats stats = SystemAPI::GetFileStats(path);
		if(stats.exists){
			FILE* file = fopen(path.c_str(), "r");
			if(file != NULL){
				uint8_t* fileData = new uint8_t[stats.size];
				fread(fileData, 1, stats.size, file);
				tile->data = fileData;
				tile->dataSize = stats.size;
				fclose(file);
				tile->isReady = true;
				return tile;
			}
		}
	}
	if(dynamicCache != ""){
		std::string path = dynamicCache + tile->fileName;
		//fprintf(stderr, "%s", path.c_str());
		SystemAPI::FileStats stats = SystemAPI::GetFileStats(path);
		if(stats.exists){
			FILE* file = fopen(path.c_str(), "r");
			if(file != NULL){
				uint8_t* fileData = new uint8_t[stats.size];
				fread(fileData, 1, stats.size, file);
				tile->data = fileData;
				tile->dataSize = stats.size;
				fclose(file);
				tile->isReady = true;
				return tile;
			}
		}
	}
	// TODO: dynamic cache and http
	tile->isReady = true;
	
	return tile;
}

void TileProvider::SetCache(std::string staticCacheFolder, std::string dynamicCacheFolder) {
	staticCache = staticCacheFolder;
	if(staticCache != "" && staticCache.back() != '/' && staticCache.back() != '\\'){
		staticCache = staticCache + "/";
	}
	dynamicCache = dynamicCacheFolder;
	if(dynamicCache != "" && dynamicCache.back() != '/' && dynamicCache.back() != '\\'){
		dynamicCache = dynamicCache + "/";
	}
	if(dynamicCache != ""){
		SystemAPI::CreateDirectory(dynamicCache);
	}
	
}

void TileProvider::EventLoop(){
}
