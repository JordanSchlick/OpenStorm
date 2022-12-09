
// asynchronously loads radar data on a separate thread
#include "RadarDataHolder.h"
#include "RadarCollection.h"


// this thing is like the Daytona 500, there are no locks to be found
class RadarLoader : public AsyncTaskRunner{
public:
	std::string path;
	RadarCollection::RadarDataSettings radarSettings;
	RadarDataHolder* radarHolder;
	uint64_t initialUid = 0;
	RadarLoader(RadarFile fileInfo, RadarCollection::RadarDataSettings radarDataSettings, RadarDataHolder* radarDataHolder){
		path = fileInfo.path;
		radarSettings = radarDataSettings;
		radarHolder = radarDataHolder;
		initialUid = RadarDataHolder::CreateUID();
		radarHolder->fileInfo = fileInfo;
		radarHolder->uid = initialUid;
		radarHolder->loader = this;
		radarHolder->state = RadarDataHolder::State::DataStateLoading;
		Start();
	}
	void Task(){
		RadarData* radarData = new RadarData();
		radarData->compress = true;
		radarData->radiusBufferCount = radarSettings.radiusBufferCount;
		radarData->thetaBufferCount = radarSettings.thetaBufferCount;
		radarData->sweepBufferCount = radarSettings.sweepBufferCount;
		//radarData->ReadNexrad(path.c_str());
		void* nexradData = radarData->ReadNexradData(path.c_str());
		radarData->LoadNexradVolume(nexradData, RadarData::VOLUME_REFLECTIVITY);
		radarData->FreeNexradData(nexradData);
		if(!canceled && initialUid == radarHolder->uid){
			radarHolder->radarData = radarData;
			radarHolder->state = RadarDataHolder::State::DataStateLoaded;
			radarHolder->loader = NULL;
		}else{
			delete radarData;
		}
	}
};

uint64_t RadarDataHolder::CreateUID(){
	static uint64_t uid = 1;
	return uid++;
}


RadarDataHolder::RadarDataHolder()
{
	uid = RadarDataHolder::CreateUID();
}

RadarDataHolder::~RadarDataHolder(){
	Unload();
}

void RadarDataHolder::Load(RadarFile file){
	asyncTasks.push_back(new RadarLoader(
		file,
		collection != NULL ? collection->radarDataSettings : RadarCollection::RadarDataSettings(),
		this
	));
}

void RadarDataHolder::Unload() {
	if(loader != NULL){
		loader->Cancel();
		loader = NULL;
	}
	for(auto task : asyncTasks){
		task->Cancel();
		task->Delete();
	}
	asyncTasks.clear();
	uid = CreateUID();
	state = RadarDataHolder::State::DataStateUnloaded;
	fileInfo = {};
	if(radarData != NULL){
		delete radarData;
		radarData = NULL;
	}
}