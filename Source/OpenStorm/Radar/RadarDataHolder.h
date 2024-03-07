#pragma once

#include "RadarData.h"
#include "AsyncTask.h"

#include <map>
#include <vector>
#include <string>
#include <atomic>

class RadarCollection;
class RadarProduct;

// represents a radar file on the disk
class RadarFile{
public:
	// time that the radar was scanned
	double time = 0;
	// time that the file was last modified
	double mtime = 0;
	// size of the file
	size_t size = 0;
	// path to file on disk
	std::string path = "";
	// name of file
	std::string name = "";
	
	// get unix timestamp from the name of a file
	static double ParseFileNameDate(std::string filename);
};

// settings for radar data that is read in
class RadarDataSettings{
public:
	// define standard max size for radar
	int radiusBufferCount = 1832;
	int thetaBufferCount = 720;
	int sweepBufferCount = 15;
	
	// type of volume to display
	RadarData::VolumeType volumeType = RadarData::VOLUME_REFLECTIVITY;
};


// a class that holds the radar products and related information
// it also manages loading in radar files and products asynchronously
class RadarDataHolder{
public:
	enum State{
		DataStateUnloaded,
		DataStateLoading,
		DataStateLoaded,
		DataStateFailed
	};
	
	// holds info about a product and the radar data related to it
	class ProductHolder {
	public:
		// volume data
		RadarData* radarData = NULL;
		// the product that is being held
		RadarProduct* product = NULL;
		// type of volume
		RadarData::VolumeType volumeType;
		// if final data that should be kept around
		bool isFinal = false;
		// if the data is going to be used for other products
		bool isDependency = false;
		// if the data has been fully loaded and ready for use
		bool isLoaded = false;
		// increment the reference counter to prevent free while in use, must call StopUsing
		void StartUsing();
		// decrement the reference counter to allow free
		void StopUsing();
		// delete the product if/once it is no longer in use
		void Delete();
		
		// do not set directly, reference counter if the data is currently being used to determine if it is safe to free
		std::atomic<int> inUse = 0;
		// should StopUsing free this product when inUse reaches 0
		bool shouldFree = false;
	private:
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
	
	RadarDataSettings radarDataSettings;
	
	RadarDataHolder();
	~RadarDataHolder();
	
	// load file, existing data will be kept, you probably want to call unload before this
	void Load(RadarFile file);
	// load data such as newly selected product, existing data will be kept
	void Load();
	// unload data and stop any loading in progress
	void Unload();
	// get a product to be loaded or add it if it is not found, should be called before Load if adding new product
	ProductHolder* GetProduct(RadarData::VolumeType type);
	// create a unique ID for use with threading
	static uint64_t CreateUID();
	
protected:
	// Store of async tasks to allow cleanup
	std::vector<AsyncTaskRunner *> asyncTasks = {};
};