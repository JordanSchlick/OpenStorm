#include <string>
#include <functional>
#include "HTTPRequest.h"

class TileProvider;

class Tile{
public:
	std::function<void()> readyCallback = NULL;
	bool isReady = false;
	uint8_t* data = NULL;
	int dataSize = 0;
	TileProvider* tileProvider;
	std::string fileName;

	// binary data for tile if available
	//uint8_t* GetData(int* size);
	
	// set the callback to be called when tile is loaded or failed
	void SetCallback(std::function<void()> callback);
	// free data and cancel loading if in progress
	~Tile();
};

class TileProvider{
public:
	HTTPPool httpPool;
	std::string name;
	std::string url;
	std::string imageType;
	// use set cache
	std::string staticCache;
	// use set cache
	std::string dynamicCache;
	int maxZoom;
	
	TileProvider(std::string name, std::string url, std::string imageType, int maxZoom);
	// set the folders to be used for caching tile image data
	// the static cache is meant for pre-downloaded data and the dynamicCacheFolder will have any tiles written to it
	// empty strings will disable the caches
	void SetCache(std::string staticCacheFolder, std::string dynamicCacheFolder);
	// get a tile for a given location, it needs to be manually deleted
	Tile* GetTile(int zoom, int y, int x);
	void EventLoop();
};