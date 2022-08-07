#pragma once

#include "RadarData.h"
#include "AsyncTask.h"

#include <string>
#include <vector>
#include <functional>


class RadarCollection{
public:
	
	// represents a file on the disk
	class RadarFile{
	public:
		double time;
		std::string path = "";
	};
	
	// a class that holds the radar data and related information
	class RadarDataHolder{
	public:
		enum State{
			DataStateUnloaded,
			DataStateLoading,
			DataStateLoaded
		};
		// uid to distinguish when it has been chenged
		uint64_t uid = 0;
		// state of loading
		State state = DataStateUnloaded;
		// radar data reference
		RadarData* radarData = NULL;
		// async loader reference
		AsyncTaskRunner* loader = NULL;
		// constructor
		RadarDataHolder();
		// destructor
		~RadarDataHolder();
		// unload data and stop any loading in progress
		void Unload();
	};
	
	// settings for radar data that is read in
	class RadarDataSettings{
	public:
		// define standard max size for radar
		int radiusBufferCount = 1832;
		int thetaBufferCount = 720;
		int sweepBufferCount = 15;
	};
	
	RadarDataSettings radarDataSettings = {};
	
	// location of directory to load from
	std::string filePath = "";
	
	int maxLoading = 100;
	
	
	static uint64_t CreateUID();
	
	RadarCollection();
	
	~RadarCollection();
	
	void Allocate(int cacheSize);
	
	void Move(int delta);
	
	void ReadFiles();
	
	void UnloadOldData();
	
	void LoadNewFiles();
	
	void LogState();
	
	
//private:	

	bool allocated = false;

	// holds currently laoded radar data
	RadarDataHolder* cache = NULL;
	
	// size of cache, expected to be even with one spot for the current data and one empty spot at all times, should be a minimum of 4
	int cacheSize = 0;
	
	// size of one side of cache when balanced excluding curent pos and empty spot
	int cacheSizeSide = 0;
	
	// number of items that should be reserved in either direction from being to reallocated to the other direction
	int reservedCacheSize = 0;
	
	// current position in the cache buffer
	int currentPosition = 0;
	
	// number of items cached before the current position
	int cachedBefore = 0;
	
	// number of items cached after the current position
	int cachedAfter = 0;
	
	// 1 for forward -1 for backward, the given direction will be prioritized for loading
	int lastMoveDirection = 1;
	
	
	float firstItemTime = -1;
	int firstItemIndex = -1;
	
	float lastItemTime = -1;
	int lastItemIndex = -1;
	
	std::vector<AsyncTaskRunner *> asyncTasks = std::vector<AsyncTaskRunner *>();
	
	std::vector<RadarFile> radarFiles = std::vector<RadarFile>();
	
	std::vector<std::function<void()>> listeners = std::vector<std::function<void()>>();
	
	static void Testing();
};