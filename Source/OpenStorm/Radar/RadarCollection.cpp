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
	fileInfo = {};
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
	RadarLoader(RadarCollection::RadarFile fileInfo, RadarCollection::RadarDataSettings radarDataSettings, RadarCollection::RadarDataHolder* radarDataHolder){
		path = fileInfo.path;
		radarSettings = radarDataSettings;
		radarHolder = radarDataHolder;
		initialUid = RadarCollection::CreateUID();
		radarHolder->fileInfo = fileInfo;
		radarHolder->uid = initialUid;
		radarHolder->loader = this;
		radarHolder->state = RadarCollection::RadarDataHolder::State::DataStateLoading;
		Start();
	}
	void Task(){
		RadarData* radarData = new RadarData();
		radarData->compress = true;
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
		
		#ifdef _WIN32
		_sleep(timeout * 1000);
		#endif
		
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
	Clear();
	if(cache != NULL){
		delete[] cache;
	}
}

void RadarCollection::Clear() {
	for(auto& item : asyncTasks){
		item->Cancel();
		item->Delete();
	}
	asyncTasks.clear();
	radarFiles.clear();
	radarFilesCache.clear();
	if(cache != NULL){
		for(int i = 0; i < cacheSize; i++){
			cache[i].Unload();
		}
	}
	// reset variables
	currentPosition = cacheSize / 2;
	cachedAfter = 0;
	cachedBefore = 0;
	lastMoveDirection = 1;
	firstItemTime = -1;
	firstItemIndex = -1;
	lastItemTime = -1;
	lastItemIndex = -1;
	needToEmit = true;
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

void RadarCollection::MoveManual(int delta) {
	Move(delta);
	lastMoveDirection = delta < 0 ? -1 : 1;
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
	double now = SystemAPI::CurrentTime();
	if(automaticallyAdvance){
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
	if(poll){
		if(nextPollTime <= now){
			nextPollTime = now + pollInterval;
			PollFiles();
		}
	}else{
		nextPollTime = 0;
	}
}

void RadarCollection::RegisterListener(std::function<void(RadarUpdateEvent)> callback) {
	listeners.push_back(callback);
}

RadarCollection::RadarDataHolder* RadarCollection::GetCurrentRadarData() {
	return &cache[currentPosition];
}

void RadarCollection::ReadFiles(std::string path) {
	std::string lastCharecter = path.substr(path.length() - 1,1);
	bool isDirectory = lastCharecter == "/" || lastCharecter == "\\";
	int lastSlash = std::max(std::max((int)path.find_last_of('/'), (int)path.find_last_of('\\')), 0);
	filePath = path.substr(0, lastSlash + 1);
	std::string inputFilename = path.substr(lastSlash + 1);
	//fprintf(stderr, "lastSlash var %i\n", (int)lastSlash);
	//fprintf(stderr, "Path var %s\n", path.c_str());
	//fprintf(stderr, "filePath var %s\n", filePath.c_str());
	
	Clear();
	PollFiles(isDirectory ? "" : inputFilename);
}

void RadarCollection::PollFiles(std::string defaultFilename) {
	if(filePath == ""){
		// no path to read from
		return;
	}
	std::vector<RadarFile> radarFilesNew = {};
	auto files = SystemAPI::ReadDirectory(filePath);
	//fprintf(stderr, "files %i\n", (int)files.size());
	float aReallyBadWayOfAssigningAnArbitraryTime = 1;
	double now = SystemAPI::CurrentTime();
	//fprintf(stderr, "now: %f\n", now);
	int fileIndex = -1;
	int lastFileIndex = files.size() - 1;
	for (auto filename : files) {
		fileIndex++;
		//fprintf(stderr, "%s\n", (filePath + filename).c_str());
		if(filename == ".gitkeep"){
			continue;
		}
		std::string path = filePath + filename;
		
		if(radarFilesCache.find(filename) != radarFilesCache.end()){
			RadarFile radarFile = radarFilesCache[filename];
			radarFile.time = aReallyBadWayOfAssigningAnArbitraryTime++;
			if(now - radarFile.mtime < 3600 || fileIndex == lastFileIndex){
				// only stat files that have been modified in the last hour or is the last one for changes
				//fprintf(stderr, "file: %f %f\n", now, radarFile.mtime);
				SystemAPI::FileStats stats = SystemAPI::GetFileStats(path);
				if(stats.isDirectory){
					continue;
				}
				radarFile.mtime = stats.mtime;
				radarFile.size = stats.size;
			}
			radarFilesNew.push_back(radarFile);
		}else{
			RadarFile radarFile = {};
			radarFile.path = path;
			radarFile.name = filename;
			radarFile.time = aReallyBadWayOfAssigningAnArbitraryTime++;
			SystemAPI::FileStats stats = SystemAPI::GetFileStats(path);
			if(stats.isDirectory){
				continue;
			}
			radarFile.size = stats.size;
			radarFile.mtime = stats.mtime;
			radarFilesNew.push_back(radarFile);
			radarFilesCache[filename] = radarFile;
		}
	}
	
	// TODO: read propper file time and sort accordingly
	
	bool moveToEnd = false;
	
	if(radarFiles.size() > 0){
		// TODO: implement reloading files by replacing radarFiles and changing firstItemIndex and lastItemIndex to reflect the new vector
		//fprintf(stderr, "file reloading is not implemented yet\n");
		//return;
		// keep current location at the end if it is there before loading new files
		moveToEnd = lastItemIndex == radarFiles.size() - 1 && cachedAfter == 0;
		std::string firstItemName = radarFiles[firstItemIndex].name;
		std::string lastItemName = radarFiles[lastItemIndex].name;
		int newFirstItemIndex = -1;
		int newLastItemIndex = -1;
		int filesCountNew = radarFilesNew.size();
		int offset = firstItemIndex;
		for(int i = 0; i < filesCountNew && (newFirstItemIndex == -1 || newLastItemIndex == -1); i++){
			// start with offset so minimal loops are needed if files are just being added to a large directory
			int location = (i + offset) % filesCountNew;
			RadarFile* fileNew = &radarFilesNew[location];
			if(newFirstItemIndex == -1 && firstItemName == fileNew->name){
				newFirstItemIndex = location;
			}
			if(newLastItemIndex == -1 && lastItemName == fileNew->name){
				newLastItemIndex = location;
			}
		}
		if(lastItemIndex - firstItemIndex == newLastItemIndex - newFirstItemIndex){
			// nothing has been added or removed form the loaded range so just update indexes
			int indexOffset = newLastItemIndex - lastItemIndex;
			// reload files whose sizes have changed starting from currentPosition and going forward
			for(int i = 0; i <= cachedAfter; i++){
				int location = lastItemIndex - cachedAfter + i;
				if(radarFiles[location].size != radarFilesNew[location + indexOffset].size){
					ReloadFile((currentPosition + i) % cacheSize);
				}
			}
			// reload files whose sizes have changed in the other direction from currentPosition
			for(int i = 1; i <= cachedBefore; i++){
				int location = firstItemIndex + cachedBefore - i;
				if(radarFiles[location].size != radarFilesNew[location + indexOffset].size){
					ReloadFile((currentPosition - i + cacheSize) % cacheSize);
				}
			}
			lastItemIndex = newLastItemIndex;
			firstItemIndex = newFirstItemIndex;
			radarFiles = radarFilesNew;
		}else{
			// loaded range has been modified on disk and needs to be completely reloaded
			Clear();
			radarFiles = radarFilesNew;
			lastItemIndex = radarFiles.size() - 1;
			firstItemIndex = lastItemIndex;
		}
	}else{
		radarFiles = radarFilesNew;
		if(lastItemIndex == -1){
			if(defaultFilename != ""){
				// start on chosen file
				int index = 0;
				for (auto file : radarFiles) {
					if(file.name == defaultFilename){
						lastItemIndex = index;
						firstItemIndex = lastItemIndex;
						break;
					}
					index++;
				}
			}
			if(lastItemIndex == -1){
				lastItemIndex = radarFiles.size() - 1;
				firstItemIndex = lastItemIndex;
			}
		}
	}
	UnloadOldData();
	LoadNewData();
	if(moveToEnd && cachedAfter > 0){
		// a better place for keeping the current position at the end might be in LoadNewData with a global state set on Move
		Move(cachedAfter);
	}
}

void RadarCollection::ReloadFile(int index) {
	if(index < 0){
		index = currentPosition;
	}
	RadarDataHolder* holder = &cache[index];
	if(holder->state != RadarDataHolder::State::DataStateUnloaded){
		fprintf(stderr, "Reloading file\n");
		holder->Unload();
		asyncTasks.push_back(new RadarLoader(
			holder->fileInfo,
			radarDataSettings,
			holder
		));
		if(index == currentPosition){
			needToEmit = true;
		}
	}
}

void RadarCollection::UnloadOldData() {
	if(!allocated){
		fprintf(stderr,"Not allocated\n");
		return;
	}
	
	//LogState();
	
	
	int maxSideSize = (cacheSizeSide + cacheSizeSide - reservedCacheSize);
	//fprintf(stderr, "remove %i %i %i\n",cachedBefore,cachedAfter,maxSideSize);
	
	int amountToRemoveBefore = cachedBefore - maxSideSize;
	//fprintf(stderr, "amountToRemoveBefore %i\n", amountToRemoveBefore);
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
	//fprintf(stderr, "amountToRemoveAfter %i\n", amountToRemoveAfter);
	if(amountToRemoveAfter > 0){
		for(int i = 0; i < amountToRemoveAfter; i++){
			RadarDataHolder* holder = &cache[modulo(currentPosition + cachedAfter, cacheSize)];
			holder->Unload();
			lastItemIndex--;
			cachedAfter--;
		}
		lastItemTime = radarFiles[lastItemIndex].time;
	}
	
	//LogState();
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
		// load current position if not loaded
		asyncTasks.push_back(new RadarLoader(
			radarFiles[lastItemIndex - cachedAfter],
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
	// iterate outward starting from both sides of the current position to find slots in the cache to load
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
					// already loaded or loading
					availableSlots -= 1;
				}else{
					if(lastItemIndex - cachedAfter + loopRun < radarFiles.size()){
						// load next file
						RadarFile file = radarFiles[lastItemIndex - cachedAfter + loopRun];
						asyncTasks.push_back(new RadarLoader(
							file,
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
					// already loaded or loading
					availableSlots -= 1;
				}else{
					if(firstItemIndex + cachedBefore - loopRun >= 0){
						// load previous file
						RadarFile file = radarFiles[firstItemIndex + cachedBefore - loopRun];
						asyncTasks.push_back(new RadarLoader(
							file,
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
	if(verbose){
		LogState();
	}
	//new LogStateDelayed(this,1);
	//new LogStateDelayed(this,3);
	//new LogStateDelayed(this,5);
	//new LogStateDelayed(this,10);
}

void RadarCollection::Emit(RadarDataHolder* holder) {
	if(verbose){
		fprintf(stderr, "Emitting %s\n", holder->fileInfo.path.c_str());
		LogState();
	}
	RadarUpdateEvent event = {};
	event.data = holder->radarData;
	event.minTimeTillNext = 0.2;
	if(automaticallyAdvance){
		event.minTimeTillNext = autoAdvanceInterval;
	}
	for(auto listener : listeners){
		listener(event);
	}
}

std::string RadarCollection::StateString(){
	std::string state = "Radar Cache: [";
	for(int i = 0; i < cacheSize; i++){
		switch(cache[i].state){
			case RadarDataHolder::State::DataStateUnloaded:
				if(i == currentPosition){
					state += ",";
				}else{
					state += ".";
				}
				break;
			case RadarDataHolder::State::DataStateLoading:
				if(i == currentPosition){
					state += "l";
				}else{
					state += "-";
				}
				break;
			case RadarDataHolder::State::DataStateLoaded:
				if(i == currentPosition){
					state += "|";
				}else{
					state += "=";
				}
				break;
		}
	}
	state += "]";
	return state;
}


std::vector<RadarCollection::RadarDataHolder::State> RadarCollection::StateVector() {
	std::vector<RadarCollection::RadarDataHolder::State> stateVector = std::vector<RadarCollection::RadarDataHolder::State>(cacheSize);
	for(int i = 0; i < cacheSize; i++){
		stateVector[i] = cache[i].state;
	}
	return stateVector;
}


int RadarCollection::GetCurrentPosition() {
	return currentPosition;
}

void RadarCollection::LogState() {
	/*fprintf(stderr,"Radar Cache: [");
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
	fprintf(stderr,"]\n");*/
	fprintf(stderr, "%s\n", StateString().c_str());
}

class AsyncTaskTest : public AsyncTaskRunner{
	void Task(){
		fprintf(stderr,"Starting task\n");
		#ifdef _WIN32
		_sleep(1000);
		#endif
		fprintf(stderr,"Ending task\n");
	}
};


void RadarCollection::Testing() {
	AsyncTaskTest* task = new AsyncTaskTest();
	task->Start(true);
}
