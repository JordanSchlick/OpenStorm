#pragma once

#include "RadarData.h"
#include "AsyncTask.h"

#include <map>
#include <vector>
#include <string>

class RadarCollection;
class RadarProduct;

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
	
	// holds info about a product and the data related to it
	class ProductHolder {
	public:
		// volume data
		RadarData* radarData = NULL;
		// the product that is being held
		RadarProduct* product= NULL;
		// type of volume
		RadarData::VolumeType volumeType;
		// if final data that should be kept around
		bool isFinal = false;
		// if the data is going to be used for other products
		bool isDependency = false;
		// if the data has been fully loaded and ready for use
		bool isLoaded = false;
		
		~ProductHolder();
	};
	
	// uid to distinguish when it has been changed
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
	// all products loaded or to be loaded
	std::vector<ProductHolder*> products;
	// map of radar products that point to products
	std::map<RadarData::VolumeType, ProductHolder*> productsMap;
	
	RadarDataHolder();
	~RadarDataHolder();
	
	//
	// load data, existing data will be kept
	void Load(RadarFile file);
	// unload data and stop any loading in progress
	void Unload();
	// get a product to be loaded or add it if it is not found
	ProductHolder* GetProduct(RadarData::VolumeType type);
	// create a unique ID for use with threading
	static uint64_t CreateUID();
	
protected:
	// Store of async tasks to allow cleanup
	std::vector<AsyncTaskRunner *> asyncTasks = {};
};