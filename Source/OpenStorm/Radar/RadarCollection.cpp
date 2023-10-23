#include "RadarCollection.h"
#include "AsyncTask.h"
#include "SystemAPI.h"


#include <algorithm>
#include <ctime>

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}

// compares if argument larger is larger than argument smaller within a looped space
inline bool moduloCompare(int smaller, int larger, int n) {
	int offset = n/2 - smaller;
	return modulo(smaller + offset,n) < modulo(larger + offset,n);
}

inline float clamp(float value, float minValue, float maxValue){
	return std::min(std::max(value, minValue), maxValue);
}
inline uint64_t clamp(uint64_t value, uint64_t minValue, uint64_t maxValue){
	return std::min(std::max(value, minValue), maxValue);
}


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
		
		SystemAPI::Sleep(timeout);
		
		collection->LogState();
	}
};

// task for asynchronously reading file list from disk
class AsyncPollFilesTask : public AsyncTaskRunner {
public:
	class CacheData {
		public:
		RadarFile radarFile;
		// index of file in the vector
		size_t index = 0;
		size_t seenOnRunId = 0;
		size_t changedOnRunId = 0;
	};
	
	// RadarCollection* collection;
	// location of directory to load from
	std::string filePath = "";
	// file to initially start on when loading directory
	std::string defaultFileName = "";
	// map used to speed up the reloading large directories
	std::unordered_map<std::string, CacheData> radarFilesCache = {};
	// new array of files
	std::vector<RadarFile> radarFilesNew = {};
	// if the new array of files is ready for use
	bool ready = false;
	// if any files have changed since the last poll
	bool hasChanged = false;
	// incremented every run to know when a CacheData is old
	size_t runId = 1;
	// size number of files found on the last run
	size_t fileCount = 0;
	// index of starting file
	size_t defaultFileIndex = 0;
	
	// AsyncPollFilesTask(RadarCollection* radarCollection){
	// 	collection = radarCollection;
	// }
	
	void Task(){
		ready = false;
		hasChanged = false;
		runId++;
		defaultFileIndex = (size_t)-1;
		radarFilesNew.clear();
		// double benchTime;
		// benchTime = SystemAPI::CurrentTime();
		auto files = SystemAPI::ReadDirectory(filePath);
		// benchTime = SystemAPI::CurrentTime() - benchTime;
		// fprintf(stderr, "dir list time %lf\n", benchTime);
		
		// benchTime = SystemAPI::CurrentTime();
		std::sort(files.begin(), files.end());
		// benchTime = SystemAPI::CurrentTime() - benchTime;
		// fprintf(stderr, "sort time %lf\n", benchTime);
		//fprintf(stderr, "files %i\n", (int)files.size());
		
		// benchTime = SystemAPI::CurrentTime();
		double now = SystemAPI::CurrentTime();
		//fprintf(stderr, "now: %f\n", now);
		size_t lastFileIndex = files.size() - 1;
		size_t finalFileIndex = 0;
		for (size_t fileIndex = 0; fileIndex <= lastFileIndex; fileIndex++) {
			SystemAPI::FileStats file = files[fileIndex];
			std::string filename = file.name;
			//fprintf(stderr, "%s\n", (filePath + filename).c_str());
			if(file.isDirectory || filename == ".gitkeep"){
				continue;
			}
			size_t extensionLocation = filename.find_last_of('.');
			if(extensionLocation != std::string::npos){
				std::string fileExtension = filename.substr(extensionLocation);
				std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), [](unsigned char c){ return std::tolower(c); });
				if(fileExtension == ".miniraddata"){
					continue;
				}
			}
			std::string path = filePath + filename;
			
			CacheData* cacheData = NULL;
			if(radarFilesCache.find(filename) != radarFilesCache.end()){
				cacheData = &radarFilesCache[filename];
				if(now - cacheData->radarFile.mtime < 3600 || fileIndex == lastFileIndex){
					// only stat files that have been modified in the last hour or is the last one for changes
					//fprintf(stderr, "file: %f %f\n", now, radarFile.mtime);
					SystemAPI::FileStats stats = SystemAPI::GetFileStats(path);
					if(stats.isDirectory){
						continue;
					}
					if(cacheData->radarFile.size != stats.size || cacheData->radarFile.mtime != stats.mtime){
						cacheData->radarFile.mtime = stats.mtime;
						cacheData->radarFile.size = stats.size;
						cacheData->changedOnRunId = runId;
						hasChanged = true;
					}
				}
				radarFilesNew.push_back(cacheData->radarFile);
			}else{
				RadarFile radarFile = {};
				radarFile.path = path;
				radarFile.name = filename;
				radarFile.time = RadarCollection::ParseFileNameDate(filename);
				radarFile.size = file.size;
				if(file.mtime > 0){
					// this mtime does not seem to perfectly line up with the file stat time which can cause files to be reloaded upon next poll
					radarFile.mtime = file.mtime;
				}if(radarFile.time > 0){
					radarFile.mtime = radarFile.time;
					// fprintf(stderr, "used parsed date %lf\n", radarFile.mtime);
				}else{
					// only stat if time was not parsed correctly
					SystemAPI::FileStats stats = SystemAPI::GetFileStats(path);
					if(stats.isDirectory){
						continue;
					}
					radarFile.size = stats.size;
					radarFile.mtime = stats.mtime;
					// fprintf(stderr, "used stat for date\n");
				}
				radarFilesNew.push_back(radarFile);
				radarFilesCache[filename] = {};
				cacheData = &radarFilesCache[filename];
				cacheData->radarFile = radarFile;
				cacheData->changedOnRunId = runId;
				hasChanged = true;
			}
			if(cacheData->index != finalFileIndex){
				hasChanged = true;
			}
			if(defaultFileName == filename){
				defaultFileIndex = finalFileIndex;
			}
			cacheData->index = finalFileIndex++;
			cacheData->seenOnRunId = runId;
		}
		if(radarFilesNew.size() != fileCount){
			hasChanged = true;
		}
		fileCount = radarFilesNew.size();
		if(fileCount == 0){
			defaultFileIndex = 0;
		}else{
			defaultFileIndex = std::min(defaultFileIndex, fileCount - 1);
		}
		// benchTime = SystemAPI::CurrentTime() - benchTime;
		// fprintf(stderr, "stat time %lf\n", benchTime);
		ready = true;
	}
};

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
	reservedCacheSizeMin = reservedCacheSize;
	reservedCacheSizeMax = cacheSizeSide;
	desiredLoadingDistance = reservedCacheSize / 2;
	currentPosition = cacheSize / 2;
	cache = new RadarDataHolder*[cacheSize];
	cacheItems = new RadarDataHolder[cacheSize];
	for(int i = 0; i < cacheSize; i++){
		cache[i] = &cacheItems[i];
		cache[i]->collection = this;
	}
	allocated = true;
}

void RadarCollection::Free() {
	allocated = false;
	Clear();
	if(cache != NULL){
		delete[] cache;
		cache = NULL;
	}
	if(cacheItems != NULL){
		delete[] cacheItems;
		cacheItems = NULL;
	}
}

void RadarCollection::Clear() {
	radarFiles.clear();
	if(pollFilesTask != NULL){
		pollFilesTask->Delete();
		pollFilesTask = NULL;
	}
	if(cache != NULL){
		for(int i = 0; i < cacheSize; i++){
			cache[i]->Unload();
		}
	}
	// reset variables
	currentPosition = cacheSize / 2;
	cachedAfter = 0;
	cachedBefore = 0;
	lastMoveDirection = 1;
	firstItemIndex = -1;
	lastItemIndex = -1;
	needToEmit = true;
}

void RadarCollection::Jump(size_t index, bool keepCurrentCachePosition) {
	if(!allocated || radarFiles.size() == 0){
		return;
	}
	index = std::min(index, radarFiles.size() - 1);
	
	
	// used holders with a filename
	std::map<std::string, RadarDataHolder*> usedHolders;
	// left over unloaded holders
	std::vector<RadarDataHolder*> unusedHolders;
	
	// add to usedHolders and unusedHolders
	for(int i = 0; i < cacheSize; i++){
		RadarDataHolder* holder = cache[i];
		if(holder->state != RadarDataHolder::DataStateUnloaded){
			std::string filename = holder->fileInfo.name;
			if(usedHolders.count(filename) > 0){
				unusedHolders.push_back(holder);
			}else{
				usedHolders[filename] = holder;
			}
		}else{
			unusedHolders.push_back(holder);
		}
	}
	
	
	
	// int64_t delta = index - currentPosition;
	// cachedBefore = clamp(cachedBefore + delta, 0, cacheSizeSide * 2);
	// cachedAfter = clamp(cachedAfter - delta, 0, cacheSizeSide * 2);
	// currentPosition = modulo(currentPosition + delta, cacheSize);
	
	// check for reusable holders
	size_t firstTestIndex = std::max((int64_t)index - cacheSize, (int64_t)0);
	size_t lastTestIndex = std::min((int64_t)index + cacheSize, (int64_t)radarFiles.size() - 1);
	size_t firstReusableIndex = index;
	size_t lastReusableIndex = index;
	for(size_t i = firstTestIndex; i <= lastTestIndex; i++){
		if(usedHolders.count(radarFiles[i].name) > 0){
			firstReusableIndex = std::min(firstReusableIndex, i);
			lastReusableIndex = std::max(lastReusableIndex, i);
		}
	}
	
	// recalculate size of sides of cache
	int targetCachedBefore = index - firstReusableIndex;
	int targetCachedAfter = lastReusableIndex - index;
	if(targetCachedBefore + targetCachedAfter > cacheSizeSide * 2){
		// range is too large and needs to be decreased
		if(targetCachedBefore < targetCachedAfter){
			targetCachedBefore = std::min(targetCachedBefore, cacheSizeSide);
			targetCachedAfter = (cacheSizeSide * 2) - targetCachedBefore;
		}else{
			targetCachedAfter = std::min(targetCachedAfter, cacheSizeSide);
			targetCachedBefore = (cacheSizeSide * 2) - targetCachedAfter;
		}
	}
	
	// clear cache before rebuilding
	for(int i = 0; i < cacheSize; i++){
		cache[i] = NULL;
	}
	
	// move current position
	int64_t delta = index - (firstItemIndex + cachedBefore);
	cachedBefore = targetCachedBefore;
	cachedAfter = targetCachedAfter;
	firstItemIndex = index - cachedBefore;
	lastItemIndex = index + targetCachedAfter;
	if(!keepCurrentCachePosition){
		currentPosition = modulo(currentPosition + delta, cacheSize);
	}
	fprintf(stderr, "%i %i %i %i\n", (int)cachedBefore, (int)cachedAfter, (int)firstItemIndex, (int)lastItemIndex);
	
	
	// move used holders back into cache
	for(int i = 0; i < targetCachedBefore + targetCachedAfter + 1; i++){
		size_t fileIndex = index - targetCachedBefore + i;
		size_t cacheIndex = modulo(currentPosition - targetCachedBefore + i, cacheSize);
		RadarFile* file = &radarFiles[fileIndex];
		if(usedHolders.count(file->name) > 0){
			// move into cache
			cache[cacheIndex] = usedHolders[file->name];
			usedHolders.erase(file->name);
		}
	}
	
	// move remaining holders into the unused array
	for(auto holder : usedHolders){
		unusedHolders.push_back(holder.second);
	}
	
	// move all unused holders into blank spots in the cache
	for(int i = 0; i < cacheSize; i++){
		if(cache[i] == NULL){
			cache[i] = unusedHolders.back();
			unusedHolders.pop_back();
			cache[i]->Unload();
		}
	}
	
	// load at new current position
	if(cache[currentPosition]->state == RadarDataHolder::DataStateUnloaded){
		cache[currentPosition]->Load(radarFiles[index]);
	}
	
	// load all unloaded ones in the range
	for(int i = 0; i < cacheSize; i++){
		if(i <= cachedBefore){
			int cacheIndex = modulo(currentPosition - i, cacheSize);
			if(cache[cacheIndex]->state == RadarDataHolder::DataStateUnloaded){
				cache[cacheIndex]->Load(radarFiles[index - i]);
			}
		}
		if(i <= cachedAfter){
			int cacheIndex = modulo(currentPosition + i, cacheSize);
			if(cache[cacheIndex]->state == RadarDataHolder::DataStateUnloaded){
				cache[cacheIndex]->Load(radarFiles[index + i]);
			}
		}
	}
	
	
	
	// load data outside of the reused range
	UnloadOldData();
	LoadNewData();
	
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
	if(cache[currentPosition]->state == RadarDataHolder::State::DataStateLoading){
		needToEmit = true;
	}else if(cache[currentPosition]->state == RadarDataHolder::State::DataStateLoaded){
		Emit(cache[currentPosition]);
		needToEmit = false;
	}
}

void RadarCollection::MoveManual(int delta) {
	Move(delta);
	lastMoveDirection = delta < 0 ? -1 : 1;
}

void RadarCollection::ChangeProduct(RadarData::VolumeType volumeType) {
	radarDataSettings.volumeType = volumeType;
	if(cache != NULL){
		cache[currentPosition]->Load();
		for(int i = 1; i < cacheSize; i++){
			if(cachedAfter >= i){
				cache[modulo(currentPosition + i, cacheSize)]->Load();
			}
			if(cachedBefore >= i){
				cache[modulo(currentPosition - i, cacheSize)]->Load();
			}
		}
	}
	needToEmit = true;
}


void RadarCollection::EventLoop() {
	if(!allocated){
		return;
	}
	//runs++;
	//if(runs > 5000){
	//	return;
	//}
	RadarDataHolder* holder = cache[currentPosition];
	if(needToEmit && holder->state == RadarDataHolder::State::DataStateLoaded){
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
			// set next animate time taking into account who late this frame is
			nextAdvanceTime = now + std::max(0.0, autoAdvanceInterval - (now - nextAdvanceTime));
		}
	}else{
		nextAdvanceTime = 0;
	}
	if(pollFilesTask != NULL && pollFilesTask->ready){
		PollFilesFinalize();
		if(pollFilesTask != NULL){
			pollFilesTask->ready = false;
		}
	}
	if(poll){
		if(nextPollTime <= now){
			PollFiles();
			nextPollTime = now + pollInterval;
		}
	}else{
		nextPollTime = 0;
	}
	for(int i = -desiredLoadingDistance; i <= desiredLoadingDistance; i++){
		int checkLocation = modulo(currentPosition + i, cacheSize);
		if(cache[checkLocation]->state == RadarDataHolder::State::DataStateLoading){
			isDesiredLoadingDistanceClear = false;
		}
	}
	if(nextReservedCacheChangeTime <= now){
		nextReservedCacheChangeTime = now + (double)(1.0f / reservedCacheChangePerSecond);
		int oldReservedCacheSize = reservedCacheSize;
		if(isDesiredLoadingDistanceClear){
			// shrink reserved size
			reservedCacheSize = std::max(reservedCacheSize - 1, reservedCacheSizeMin);
		}else{
			// expand reserved size
			reservedCacheSize = std::min(reservedCacheSize + 1, reservedCacheSizeMax);
		}
		if(oldReservedCacheSize != reservedCacheSize){
			UnloadOldData();
			LoadNewData();
		}
		isDesiredLoadingDistanceClear = true;
	}
}

void RadarCollection::RegisterListener(std::function<void(RadarUpdateEvent)> callback) {
	listeners.push_back(callback);
}

RadarDataHolder* RadarCollection::GetCurrentRadarData() {
	return cache[currentPosition];
}

void RadarCollection::ReadFiles(std::string path) {
	std::string lastCharecter = path.substr(path.length() - 1,1);
	bool isDirectory = lastCharecter == "/" || lastCharecter == "\\";
	int lastSlash = std::max(std::max((int)path.find_last_of('/'), (int)path.find_last_of('\\')), -1);
	filePath = path.substr(0, lastSlash + 1);
	std::string inputFilename = path.substr(lastSlash + 1);
	defaultFileName = isDirectory ? "" : inputFilename;
	//fprintf(stderr, "lastSlash var %i\n", (int)lastSlash);
	//fprintf(stderr, "Path var %s\n", path.c_str());
	//fprintf(stderr, "filePath var %s\n", filePath.c_str());
	
	Clear();
	PollFiles();
}

double RadarCollection::ParseFileNameDate(std::string filename) {
	std::string datePart = "";
	std::string timePart = "";
	int numberPartStartIndex = 0;
	int filenameLength = filename.length();
	for(int i = 0; i <= filenameLength; i++){
		bool notPart = i == filenameLength;
		if(!notPart){
			char character = filename[i];
			notPart = !('0' <= character && character <= '9');
		}
		if(notPart){
			int partSize = i - numberPartStartIndex;
			if(partSize > 0){
				std::string part = filename.substr(numberPartStartIndex, partSize);
				if(partSize == 8){
					int beginNumber = std::stoi(part.substr(0, 4));
					if(beginNumber > 1900 && beginNumber < 2999){
						datePart = part;
					}
				}
				if(partSize == 6){
					timePart = part;
				}
				if(partSize == 14){
					int beginNumber = std::stoi(part.substr(0, 4));
					if(beginNumber > 1900 && beginNumber < 2999){
						datePart = part.substr(0, 8);
						timePart = part.substr(8, 6);
					}
				}
				if(partSize == 12){
					int beginNumber = std::stoi(part.substr(0, 4));
					if(beginNumber > 1900 && beginNumber < 2999){
						datePart = part.substr(0, 8);
						timePart = part.substr(8, 6);
					}
				}
			}
			numberPartStartIndex = i + 1;
		}
	}
	if(datePart == "" || timePart == ""){
		return 0;
	}
	struct tm t = {0};
	t.tm_year = std::stoi(datePart.substr(0, 4)) - 1900; 
	t.tm_mon = std::stoi(datePart.substr(4, 2)) - 1;
	t.tm_mday = std::stoi(datePart.substr(6, 2));
	if(timePart.length() == 4){
		t.tm_hour = std::stoi(datePart.substr(0, 2));
		t.tm_min = std::stoi(datePart.substr(2, 2));
	}else{
		t.tm_hour = std::stoi(datePart.substr(0, 2));
		t.tm_min = std::stoi(datePart.substr(2, 2));
		t.tm_sec = std::stoi(datePart.substr(4, 2));
	}
	
	
	
	#ifdef _WIN32
	time_t timeSinceEpoch = _mkgmtime(&t);
	// fprintf(stderr, "%i %i %i %i %i %i %lli\n", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, timeSinceEpoch);
	#else
	time_t timeSinceEpoch = timegm(&t);
	#endif
	return (double)timeSinceEpoch;
}



void RadarCollection::PollFilesFinalize() {
	// double benchTime;
	
	// benchTime = SystemAPI::CurrentTime();
	bool moveToEnd = false;
	
	std::vector<RadarFile>* radarFilesNew = &pollFilesTask->radarFilesNew;
	
	// if(radarFiles.size() > 0 && firstItemIndex < radarFiles.size() && lastItemIndex < radarFiles.size()){
	// 	// TODO: implement reloading files by replacing radarFiles and changing firstItemIndex and lastItemIndex to reflect the new vector
	// 	//fprintf(stderr, "file reloading is not implemented yet\n");
	// 	//return;
	// 	// keep current location at the end if it is there before loading new files
	// 	moveToEnd = lastItemIndex == radarFiles.size() - 1 && cachedAfter == 0;
	// 	std::string firstItemName = radarFiles[firstItemIndex].name;
	// 	std::string lastItemName = radarFiles[lastItemIndex].name;
	// 	int newFirstItemIndex = -1;
	// 	int newLastItemIndex = -1;
	// 	int filesCountNew = radarFilesNew->size();
	// 	int offset = firstItemIndex;
	// 	for(int i = 0; i < filesCountNew && (newFirstItemIndex == -1 || newLastItemIndex == -1); i++){
	// 		// start with offset so minimal loops are needed if files are just being added to a large directory
	// 		int location = (i + offset) % filesCountNew;
	// 		RadarFile* fileNew = &(*radarFilesNew)[location];
	// 		if(newFirstItemIndex == -1 && firstItemName == fileNew->name){
	// 			newFirstItemIndex = location;
	// 		}
	// 		if(newLastItemIndex == -1 && lastItemName == fileNew->name){
	// 			newLastItemIndex = location;
	// 		}
	// 	}
	// 	if(lastItemIndex - firstItemIndex == newLastItemIndex - newFirstItemIndex){
	// 		// nothing has been added or removed form the loaded range so just update indexes
	// 		int indexOffset = newLastItemIndex - lastItemIndex;
	// 		// reload files whose sizes have changed starting from currentPosition and going forward
	// 		for(int i = 0; i <= cachedAfter; i++){
	// 			int location = lastItemIndex - cachedAfter + i;
	// 			if(radarFiles[location].size != (*radarFilesNew)[location + indexOffset].size){
	// 				ReloadFile((currentPosition + i) % cacheSize);
	// 			}
	// 		}
	// 		// reload files whose sizes have changed in the other direction from currentPosition
	// 		for(int i = 1; i <= cachedBefore; i++){
	// 			int location = firstItemIndex + cachedBefore - i;
	// 			if(radarFiles[location].size != (*radarFilesNew)[location + indexOffset].size){
	// 				ReloadFile((currentPosition - i + cacheSize) % cacheSize);
	// 			}
	// 		}
	// 		lastItemIndex = newLastItemIndex;
	// 		firstItemIndex = newFirstItemIndex;
	// 		radarFiles = std::move(*radarFilesNew);
	// 	}else{
	// 		// loaded range has been modified on disk and needs to be completely reloaded
	// 		Clear();
	// 		radarFiles = std::move(*radarFilesNew);
	// 		lastItemIndex = radarFiles.size() - 1;
	// 		firstItemIndex = lastItemIndex;
	// 	}
	// }else{
	// 	radarFiles = std::move(*radarFilesNew);
	// 	if(lastItemIndex == -1){
	// 		if(defaultFileName != ""){
	// 			// start on chosen file
	// 			int index = 0;
	// 			for (auto file : radarFiles) {
	// 				if(file.name == defaultFileName){
	// 					lastItemIndex = index;
	// 					firstItemIndex = lastItemIndex;
	// 					break;
	// 				}
	// 				index++;
	// 			}
	// 		}
	// 		if(lastItemIndex == -1){
	// 			lastItemIndex = radarFiles.size() - 1;
	// 			firstItemIndex = lastItemIndex;
	// 		}
	// 	}
	// }
	
	if(pollFilesTask->hasChanged){
		if(radarFiles.size() > 0 && radarFilesNew->size() > 0){
			// incorporate changes into existing cache
			
			// check existing file
			size_t oldIndex = firstItemIndex + cachedBefore;
			RadarFile oldFile = radarFiles[oldIndex];
			bool wasLast = oldIndex + 1 == radarFiles.size();
			
			// move new file list into radarFiles
			radarFiles = std::move(*radarFilesNew);
			
			// check for new index of file
			size_t newFileIndex = (size_t)-1;
			if(!wasLast && pollFilesTask->radarFilesCache.count(oldFile.name) > 0){
				AsyncPollFilesTask::CacheData cacheData = pollFilesTask->radarFilesCache[oldFile.name];
				if(cacheData.seenOnRunId == pollFilesTask->runId){
					newFileIndex = cacheData.index;
				}
			}
			
			// move to new index and rebuild cache
			Jump(newFileIndex, newFileIndex < radarFiles.size());
			
			
			// check if any of the loaded items need to be reloaded
			for(int i = 0; i < cachedBefore + cachedAfter + 1; i++){
				int index = modulo(currentPosition - cachedBefore + i, cacheSize);
				if(cache[index]->state != RadarDataHolder::DataStateUnloaded){
					RadarFile* radarFile = &radarFiles[firstItemIndex + i];
					if(pollFilesTask->radarFilesCache.count(radarFile->name)){
						// check if the file was changed
						if(pollFilesTask->radarFilesCache[radarFile->name].changedOnRunId == pollFilesTask->runId){
							cache[index]->Unload();
							cache[index]->Load(*radarFile);
						}
					}
				}
			}
		}else if(radarFilesNew->size() > 0){
			// load new data into blank cache
			radarFiles = std::move(*radarFilesNew);
			size_t startIndex = pollFilesTask->defaultFileIndex;
			firstItemIndex = startIndex;
			lastItemIndex = startIndex;
			LoadNewData();
		}else{
			// unload cache
			Clear();
		}
		
	}
	
	// benchTime = SystemAPI::CurrentTime() - benchTime;
	// fprintf(stderr, "move time %lf\n", benchTime);
	
	// benchTime = SystemAPI::CurrentTime();
	// UnloadOldData();
	// LoadNewData();
	// if(moveToEnd && cachedAfter > 0){
	// 	// a better place for keeping the current position at the end might be in LoadNewData with a global state set on Move
	// 	Move(cachedAfter);
	// }
	
	// benchTime = SystemAPI::CurrentTime() - benchTime;
	// fprintf(stderr, "load time %lf\n", benchTime);
}

void RadarCollection::PollFiles() {
	if(filePath == ""){
		// no path to read from
		return;
	}
	
	if(pollFilesTask == NULL){
		pollFilesTask = new AsyncPollFilesTask();
	}
	
	if(!pollFilesTask->running && !pollFilesTask->ready){
		pollFilesTask->filePath = filePath;
		pollFilesTask->defaultFileName = defaultFileName;
		pollFilesTask->Start();
	}
}

void RadarCollection::ReloadFile(int cacheIndex) {
	if(cacheIndex < 0){
		cacheIndex = currentPosition;
	}
	RadarDataHolder* holder = cache[cacheIndex];
	if(holder->state != RadarDataHolder::State::DataStateUnloaded){
		fprintf(stderr, "Reloading file\n");
		RadarFile info = holder->fileInfo;
		holder->Unload();
		holder->Load(info);
		if(cacheIndex == currentPosition){
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
	
	// maximum number that can be loaded in one direction
	int maxSideSize = (cacheSizeSide + cacheSizeSide - reservedCacheSize);
	//fprintf(stderr, "remove %i %i %i\n",cachedBefore,cachedAfter,maxSideSize);
	
	int amountToRemoveBefore = cachedBefore - maxSideSize;
	//fprintf(stderr, "amountToRemoveBefore %i\n", amountToRemoveBefore);
	if(amountToRemoveBefore > 0 && lastItemIndex + 1 != radarFiles.size()){
		for(int i = 0; i < amountToRemoveBefore; i++){
			RadarDataHolder* holder = cache[modulo(currentPosition - cachedBefore, cacheSize)];
			holder->Unload();
			firstItemIndex++;
			cachedBefore--;
		}
	}
	
	
	int amountToRemoveAfter = cachedAfter - maxSideSize;
	//fprintf(stderr, "amountToRemoveAfter %i\n", amountToRemoveAfter);
	if(amountToRemoveAfter > 0 && firstItemIndex != 0){
		for(int i = 0; i < amountToRemoveAfter; i++){
			RadarDataHolder* holder = cache[modulo(currentPosition + cachedAfter, cacheSize)];
			holder->Unload();
			lastItemIndex--;
			cachedAfter--;
		}
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
		switch(cache[i]->state){
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
	if(cache[currentPosition]->state == RadarDataHolder::State::DataStateUnloaded){
		// load current position if not loaded
		cache[currentPosition]->Load(radarFiles[lastItemIndex - cachedAfter]);
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
		
		// this if statement is used to alternate between sides of the curent position so that data closest to the current position will be scheduled for loading first
		// if you make a change to one section of the if statement you need to make a change the other section
		if(side == 1){
			//load forward
			if(modulo(currentPosition + loopRun, cacheSize) == modulo(currentPosition - cachedBefore - 1, cacheSize)){
				// it has collided with the empty spot
				forwardEnded = true;
			}
			if(!forwardEnded){
				RadarDataHolder* holder = cache[modulo(currentPosition + loopRun,cacheSize)];
				
				if(holder->state != RadarDataHolder::State::DataStateUnloaded){
					// already loaded or loading
					availableSlots -= 1;
				}else{
					if(lastItemIndex - cachedAfter + loopRun < radarFiles.size()){
						// load next file
						RadarFile file = radarFiles[lastItemIndex - cachedAfter + loopRun];
						holder->Load(file);
						cachedAfter++;
						lastItemIndex++;
						availableSlots -= 1;
					}else if(loopRun <= reservedCacheSize){
						// reserve for future available files
						availableSlots -= 1;
					}
				}
			}
		}else{
			//load backward
			if(modulo(currentPosition - loopRun, cacheSize) == modulo(currentPosition + cachedAfter + 1,cacheSize)){
				// it has collided with the empty spot
				backwardEnded = true;
			}
			if(!backwardEnded){
				RadarDataHolder* holder = cache[modulo(currentPosition - loopRun,cacheSize)];
				if(holder->state != RadarDataHolder::State::DataStateUnloaded){
					// already loaded or loading
					availableSlots -= 1;
				}else{
					if(firstItemIndex + cachedBefore - loopRun >= 0){
						// load previous file
						RadarFile file = radarFiles[firstItemIndex + cachedBefore - loopRun];
						holder->Load(file);
						cachedBefore++;
						firstItemIndex--;
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
		// I have never seen this printed and I hope no one ever will.
		// Actually, I think this can be reached if there are too few files
		// It has been proven that this is a fairly common occurrence so I am commening it out
		//fprintf(stderr, "You fool! You have made the loop run to long.\n");
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
		switch(cache[i]->state){
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


std::vector<RadarDataHolder::State> RadarCollection::StateVector() {
	std::vector<RadarDataHolder::State> stateVector = std::vector<RadarDataHolder::State>(cacheSize);
	for(int i = 0; i < cacheSize; i++){
		stateVector[i] = cache[i]->state;
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
		SystemAPI::Sleep(1);
		fprintf(stderr,"Ending task\n");
	}
};


void RadarCollection::Testing() {
	AsyncTaskTest* task = new AsyncTaskTest();
	task->Start(true);
}
