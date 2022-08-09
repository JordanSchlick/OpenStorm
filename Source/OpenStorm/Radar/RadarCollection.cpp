#include "RadarCollection.h"
#include "AsyncTask.h"
#include "SystemAPI.h"


#include <algorithm>

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

// compares if argument larger is larger than argument smaller within a looped space
bool moduloCompare(int smaller, int larger, int n) {
	int offset = n/2 - smaller;
	return modulo(smaller + offset,n) < modulo(larger + offset,n);
}


RadarCollection::RadarDataHolder::RadarDataHolder()
{
	uid = RadarCollection::CreateUID();
}

RadarCollection::RadarDataHolder::~RadarDataHolder(){
	Unload();
}

void RadarCollection::RadarDataHolder::Unload() {
	if(loader != NULL){
		loader->Cancel();
		loader = NULL;
	}
	uid = CreateUID();
	state = RadarDataHolder::State::DataStateUnloaded;
	filePath = "";
	if(radarData != NULL){
		delete radarData;
		radarData = NULL;
	}
}

// asynchronously loads radar data on a separate thread
// this thing is like the Daytona 500, there are no locks to be found
class RadarLoader : public AsyncTaskRunner{
public:
	std::string path;
	RadarCollection::RadarDataSettings radarSettings;
	RadarCollection::RadarDataHolder* radarHolder;
	uint64_t initialUid = 0;
	RadarLoader(std::string filePath, RadarCollection::RadarDataSettings radarDataSettings, RadarCollection::RadarDataHolder* radarDataHolder){
		path = filePath;
		radarSettings = radarDataSettings;
		radarHolder = radarDataHolder;
		initialUid = RadarCollection::CreateUID();
		radarHolder->filePath = filePath;
		radarHolder->uid = initialUid;
		radarHolder->loader = this;
		radarHolder->state = RadarCollection::RadarDataHolder::State::DataStateLoading;
		Start();
	}
	void Task(){
		RadarData* radarData = new RadarData();
		radarData->radiusBufferCount = radarSettings.radiusBufferCount;
		radarData->thetaBufferCount = radarSettings.thetaBufferCount;
		radarData->sweepBufferCount = radarSettings.sweepBufferCount;
		radarData->ReadNexrad(path.c_str());
		if(!canceled && initialUid == radarHolder->uid){
			radarHolder->radarData = radarData;
			radarHolder->state = RadarCollection::RadarDataHolder::State::DataStateLoaded;
			radarHolder->loader = NULL;
		}else{
			delete radarData;
		}
	}
};

// helper for testing
class LogStateDelayed : public AsyncTaskRunner{
public:
	RadarCollection* collection;
	float timeout;
	LogStateDelayed(RadarCollection* radarCollection, float time){
		collection = radarCollection;
		timeout = time;
		Start(true);
	}
	void Task(){
		_sleep(timeout * 1000);
		collection->LogState();
	}
};

uint64_t RadarCollection::CreateUID(){
	static uint64_t uid = 1;
	return uid++;
}

RadarCollection::RadarCollection()
{
	
}

RadarCollection::~RadarCollection()
{
	Free();
}

void RadarCollection::Allocate(int newCacheSize) {
	if(allocated){
		fprintf(stderr,"Can not reallocate\n");
		return;
	}
	// make it to a multiple of 2 and minimum of 4
	cacheSize = newCacheSize & ~1;
	cacheSize = std::max(cacheSize, 4);
	cacheSizeSide = cacheSize / 2 - 1;
	reservedCacheSize = cacheSizeSide / 2;
	reservedCacheSize = std::max(std::min(reservedCacheSize,10),std::min(cacheSizeSide,4));
	currentPosition = cacheSize / 2;
	cache = new RadarDataHolder[cacheSize];
	allocated = true;
}

void RadarCollection::Free() {
	allocated = false;
	for(auto& item : asyncTasks){
		item->Cancel();
		item->Delete();
	}
	asyncTasks.clear();
	radarFiles.clear();
	if(cache != NULL){
		delete[] cache;
	}
}

void RadarCollection::Move(int delta) {
	if(!allocated){
		return;
	}
	delta = std::max(delta,-cachedBefore);
	delta = std::min(delta,cachedAfter);
	cachedBefore += delta;
	cachedAfter -= delta;
	currentPosition = modulo(currentPosition + delta, cacheSize);
	UnloadOldData();
	LoadNewData();
	if(cache[currentPosition].state == RadarDataHolder::State::DataStateLoading){
		needToEmit = true;
	}else{
		Emit(&cache[currentPosition]);
		needToEmit = false;
	}
}


void RadarCollection::EventLoop() {
	if(!allocated){
		return;
	}
	//runs++;
	//if(runs > 5000){
	//	return;
	//}
	RadarDataHolder* holder = &cache[currentPosition];
	if(needToEmit && holder->state != RadarDataHolder::State::DataStateLoading){
		Emit(holder);
		needToEmit = false;
	}
	if(automaticallyAdvance){
		double now = SystemAPI::CurrentTime();
		if(nextAdvanceTime <= now){
			if(lastItemIndex == radarFiles.size() - 1 && cachedAfter == 0){
				lastMoveDirection = -1;
			}
			if(firstItemIndex == 0 && cachedBefore == 0){
				lastMoveDirection = 1;
			}
			if(holder->state != RadarDataHolder::State::DataStateLoading && radarFiles.size() > 1){
				Move(lastMoveDirection);
			}
			nextAdvanceTime = now + autoAdvanceInterval;
		}
	}else{
		nextAdvanceTime = 0;
	}
}

void RadarCollection::RegisterListener(std::function<void(RadarData *)> callback) {
	listeners.push_back(callback);
}

void RadarCollection::ReadFiles() {
	if(radarFiles.size() > 0){
		// TODO: implement reloading files by replacing radarFiles and changing firstItemIndex and lastItemIndex to reflect the new vector
		fprintf(stderr, "file reloading is not implemented yet\n");
		return;
	}

	auto files = SystemAPI::ReadDirectory(filePath);
	fprintf(stderr, "files %i\n", (int)files.size());
	float i = 1;
	for (auto filename : files) {
		fprintf(stderr, "%s\n", (filePath + filename).c_str());
		RadarFile radarFile = {};
		radarFile.path = filePath + filename;
		radarFile.time = i++;
		radarFiles.push_back(radarFile);
	}
	if(lastItemIndex == -1){
		lastItemIndex = radarFiles.size() - 1;
		firstItemIndex = lastItemIndex;
	}
}

void RadarCollection::UnloadOldData() {
	if(!allocated){
		fprintf(stderr,"Not allocated\n");
		return;
	}
	
	LogState();
	
	
	int maxSideSize = (cacheSizeSide + cacheSizeSide - reservedCacheSize);
	fprintf(stderr, "remove %i %i %i\n",cachedBefore,cachedAfter,maxSideSize);
	
	int amountToRemoveBefore = cachedBefore - maxSideSize;
	fprintf(stderr, "amountToRemoveBefore %i\n", amountToRemoveBefore);
	if(amountToRemoveBefore > 0){
		for(int i = 0; i < amountToRemoveBefore; i++){
			RadarDataHolder* holder = &cache[modulo(currentPosition - cachedBefore, cacheSize)];
			holder->Unload();
			firstItemIndex++;
			cachedBefore--;
		}
		firstItemTime = radarFiles[firstItemIndex].time;
	}
	
	
	int amountToRemoveAfter = cachedAfter - maxSideSize;
	fprintf(stderr, "amountToRemoveAfter %i\n", amountToRemoveAfter);
	if(amountToRemoveAfter > 0){
		for(int i = 0; i < amountToRemoveAfter; i++){
			RadarDataHolder* holder = &cache[modulo(currentPosition + cachedAfter, cacheSize)];
			holder->Unload();
			lastItemIndex--;
			cachedAfter--;
		}
		lastItemTime = radarFiles[lastItemIndex].time;
	}
	
	LogState();
}



void RadarCollection::LoadNewData() {
	if(!allocated){
		fprintf(stderr,"Not allocated\n");
		return;
	}
	if(lastItemIndex < 0){
		fprintf(stderr,"No files loaded\n");
		return;
	}
	int unloadedCount = 0;
	int loadingCount = 0;
	int loadedCount = 0;
	for(int i = 0; i < cacheSize; i++){
		switch(cache[i].state){
			case RadarDataHolder::State::DataStateUnloaded:
				unloadedCount++;
				break;
			case RadarDataHolder::State::DataStateLoading:
				loadingCount++;
				break;
			case RadarDataHolder::State::DataStateLoaded:
			case RadarDataHolder::State::DataStateFailed:
				loadedCount++;
				break;
		}
	}
	if(cache[currentPosition].state == RadarDataHolder::State::DataStateUnloaded){
		asyncTasks.push_back(new RadarLoader(
			radarFiles[lastItemIndex - cachedAfter].path,
			radarDataSettings,
			&cache[currentPosition]
		));
	}
	
	int maxLoops = cacheSize * 2;
	
	int availableSlots = cacheSizeSide * 2;
	int loopRun = 1;
	int side = lastMoveDirection;
	bool forwardEnded = false;
	bool backwardEnded = false;
	while(availableSlots > 0 && maxLoops > 0){
		//fprintf(stderr,"state %i %i %i\n",cachedBefore,cachedAfter,availableSlots);
		
		// this if statement is used to alternate between sides
		// if you make a change to one section you need to make a change the other section
		if(side == 1){
			//advance forward
			if(modulo(currentPosition + loopRun, cacheSize) == modulo(currentPosition - cachedBefore - 1, cacheSize)){
				// it has collided with the empty spot
				forwardEnded = true;
			}
			if(!forwardEnded){
				RadarDataHolder* holder = &cache[modulo(currentPosition + loopRun,cacheSize)];
				
				if(holder->state != RadarDataHolder::State::DataStateUnloaded){
					// already loaded
					availableSlots -= 1;
				}else{
					if(lastItemIndex - cachedAfter + loopRun < radarFiles.size()){
						// load next file
						RadarFile file = radarFiles[lastItemIndex - cachedAfter + loopRun];
						asyncTasks.push_back(new RadarLoader(
							file.path,
							radarDataSettings,
							holder
						));
						cachedAfter++;
						lastItemIndex++;
						lastItemTime = file.time;
						availableSlots -= 1;
					}else if(loopRun <= reservedCacheSize){
						// reserve for future available files
						availableSlots -= 1;
					}
				}
			}
		}else{
			//advance backward
			if(modulo(currentPosition - loopRun, cacheSize) == modulo(currentPosition + cachedAfter + 1,cacheSize)){
				// it has collided with the empty spot
				backwardEnded = true;
			}
			if(!backwardEnded){
				RadarDataHolder* holder = &cache[modulo(currentPosition - loopRun,cacheSize)];
				if(holder->state != RadarDataHolder::State::DataStateUnloaded){
					// already loaded
					availableSlots -= 1;
				}else{
					if(firstItemIndex + cachedBefore - loopRun >= 0){
						// load previous file
						RadarFile file = radarFiles[firstItemIndex + cachedBefore - loopRun];
						asyncTasks.push_back(new RadarLoader(
							file.path,
							radarDataSettings,
							holder
						));
						cachedBefore++;
						firstItemIndex--;
						firstItemTime = file.time;
						availableSlots -= 1;
					}else if(loopRun <= reservedCacheSize){
						// reserve for future available files
						availableSlots -= 1;
					}
				}
			}
		}
		
		if(side * -1 == lastMoveDirection){
			// move outwards every other run
			loopRun++;
		}
		// switch side each loop
		side = side * -1;
		
		maxLoops--;
	}
	
	if(maxLoops == 0){
		// I have never seen this printed and I home no one ever will.
		// Actually, I think this can be reached if there are too few files
		fprintf(stderr, "You fool! You have made the loop run to long.\n");
	}
	
	LogState();
	//new LogStateDelayed(this,1);
	//new LogStateDelayed(this,3);
	//new LogStateDelayed(this,5);
	//new LogStateDelayed(this,10);
}

void RadarCollection::Emit(RadarDataHolder* holder) {
	fprintf(stderr, "Emitting %s\n", holder->filePath.c_str());
	LogState();
	for(auto listener : listeners){
		listener(holder->radarData);
	}
}

void RadarCollection::LogState() {
	fprintf(stderr,"Radar Cache: [");
	for(int i = 0; i < cacheSize; i++){
		switch(cache[i].state){
			case RadarDataHolder::State::DataStateUnloaded:
				if(i == currentPosition){
					fprintf(stderr,",");
				}else{
					fprintf(stderr,".");
				}
				break;
			case RadarDataHolder::State::DataStateLoading:
				if(i == currentPosition){
					fprintf(stderr,"l");
				}else{
					fprintf(stderr,"-");
				}
				break;
			case RadarDataHolder::State::DataStateLoaded:
				if(i == currentPosition){
					fprintf(stderr,"|");
				}else{
					fprintf(stderr,"=");
				}
				break;
		}
	}
	fprintf(stderr,"]\n");
	
}

class AsyncTaskTest : public AsyncTaskRunner{
	void Task(){
		fprintf(stderr,"Starting task\n");
		_sleep(1000);
		fprintf(stderr,"Ending task\n");
	}
};


void RadarCollection::Testing() {
	AsyncTaskTest* task = new AsyncTaskTest();
	task->Start(true);
}
