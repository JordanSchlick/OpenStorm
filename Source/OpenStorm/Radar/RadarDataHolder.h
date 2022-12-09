#pragma once

#include "RadarData.h"
#include "AsyncTask.h"

#include <vector>
#include <string>

class RadarCollection;

// represents a radar file on the disk
class RadarFile{
public:
	// time that the radar was scanned
	double time;
	// time that the file was last modified
	double mtime;
	// size of the file
	size_t size;
	// path to file on disk
	std::string path = "";
	// name of file
	std::string name = "";
};


// a class that holds the radar data and related information
class RadarDataHolder{
public:
	enum State{
		DataStateUnloaded,
		DataStateLoading,
		DataStateLoaded,
		DataStateFailed
	};
	// uid to distinguish when it has been chenged
	uint64_t uid = 0;
	// state of loading
	State state = DataStateUnloaded;
	// radar data reference
	RadarData* radarData = NULL;
	// async loader reference
	AsyncTaskRunner* loader = NULL;
	// the RadarCollection that this belongs to, could possibly be null
	RadarCollection* collection = NULL;
	// info about the file on disk
	RadarFile fileInfo = {};
	// constructor
	RadarDataHolder();
	// destructor
	~RadarDataHolder();
	// load data, existing data will be kept
	void Load(RadarFile file);
	// unload data and stop any loading in progress
	void Unload();
	// create a UID for use with threading
	static uint64_t CreateUID();
protected:
	// Store of async tasks to allow cleanup
	std::vector<AsyncTaskRunner *> asyncTasks = {};
};