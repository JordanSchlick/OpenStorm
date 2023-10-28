#include "RadarDataDownloader.h"

#include "../Radar/AsyncTask.h"
#include "../Radar/SystemAPI.h"
#include "../Objects/RadarGameStateBase.h"
#include "../EngineHelpers/StringUtils.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Engine/World.h"

#include <algorithm>
#include <string>
#include <vector>

class HTTPDownloader{
public:
	// if not blank the request will be written to this file
	std::string outputFile = "";
	// location to start downloading, -1 for whole file
	int64_t firstByte = -1;
	// location to download, -1 for rest of file
	int64_t lastByte = -1;
	// response body
	uint8_t* buffer = NULL;
	// response body size
	size_t bufferSize = 0;
	// seconds before request times out
	float timeout = 10;
	// if complete
	bool done = false;
	// if it was successful
	bool success = false;
	// reason the request failed
	std::string error = "";
	// reference to http response to prevent buffer from being freed
	FHttpResponsePtr httpResponsePtr;
	bool hasReceivedContentRangeHeader = false;
	bool hasReceivedAnyHeaders = false;
	// start the request and wait for it to finish, will stop early if canceled becomes true
	void MakeRequest(std::string url, bool* canceled){
		Clear();
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> httpRequest = FHttpModule::Get().CreateRequest();
		httpRequest->SetURL(StringUtils::STDStringToFString(url));
		httpRequest->SetVerb("GET");
		httpRequest->SetTimeout(timeout);
		if(firstByte >= 0){
			if(lastByte >= 0){
				httpRequest->SetHeader("Range", "bytes=" + FString::FromInt(firstByte) + "-" + FString::FromInt(lastByte));
			}else{
				httpRequest->SetHeader("Range", "bytes=" + FString::FromInt(firstByte) + "-");
			}
		}
		
		httpRequest->OnRequestWillRetry().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, float SecondsToRetry){
			// this should never happen
			fprintf(stderr, "OnRequestWillRetry\n");
		});
		httpRequest->OnHeaderReceived().BindLambda([this](FHttpRequestPtr Request, const FString& HeaderName, const FString& NewHeaderValue){
			if (HeaderName == TEXT("Content-Range")){
				hasReceivedContentRangeHeader = true;
			}
			hasReceivedAnyHeaders = true;
			//fprintf(stderr, "OnHeaderReceived\n");
		});
		httpRequest->OnRequestProgress().BindLambda([this](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived){
			// this could be problematic if the header ever gets split because this event fires before the header is parsed
			// if the second part of the header contains a large amount of data this could fire before the second part is parsed
			// that is probably extremely unlikely to happen though
			if(firstByte >= 0 && hasReceivedAnyHeaders && !hasReceivedContentRangeHeader && BytesReceived > 5000){
				// the request never had a Content-Range header despite being requested to have one
				// the request has now started to download more than requested so it needs to die
				Request->CancelRequest();
				error = "the request was canceled because it was not returning a range";
				fprintf(stderr, "%s\n", error.c_str());
			}
			//fprintf(stderr, "OnRequestProgress %i\n", BytesReceived);
		});
		httpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess){
			fprintf(stderr, "OnProcessRequestComplete %i\n", bSuccess);
			if (bSuccess && Response.IsValid()){
				httpResponsePtr = Response;
				buffer = (uint8_t*)Response->GetContent().GetData();
				bufferSize = Response->GetContent().Num();
				int responseCode = Response->GetResponseCode();
				fprintf(stderr, "GetResponseCode %i\n", responseCode);
				if(firstByte < 0 && responseCode != 200){
					// reject if not 200 for normal download
					error = "did not get successful error code, expected 200 but got " + std::to_string(responseCode);
					done = true;
					return;
				} 
				if(firstByte >= 0 && responseCode != 206){
					// reject if not 206 when a range was requested
					// a 200 means that it is returning the whole thing instead
					error = "did not get successful error code, expected 206 but got " + std::to_string(responseCode);
					done = true;
					return;
				}
				if(outputFile != ""){
					if(firstByte < 0 || !SystemAPI::GetFileStats(outputFile).exists){
						// write new file
						FILE* file = fopen(outputFile.c_str(), "w+");
						if(file){
							fwrite(buffer, 1, bufferSize, file);
							fclose(file);
							success = true;
						}else{
							error = "failed to open output file";
						}
					}else{
						// add section to file
						FILE* file = fopen(outputFile.c_str(), "r+");
						if(file){
							fseek(file, firstByte, SEEK_SET);
							fwrite(buffer, 1, bufferSize, file);
							fclose(file);
							success = true;
						}else{
							error = "failed to open output file";
						}
					}
				}else{
					success = true;
				}
			}else{
				error = "request failed";
			}
			
			done = true;
		});
		httpRequest->ProcessRequest();
		// block until the thread until the request is done or the request is canceled
		while(!done){
			if(*canceled){
				httpRequest->CancelRequest();
				// dont exit until the request is completely done to prevent external code from freeing this object before the request callbacks finish
				SystemAPI::Sleep(0.02);
			}else{
				SystemAPI::Sleep(0.1);
			}
		}
	}
	void Clear(){
		done = false;
		success = false;
		buffer = NULL;
		bufferSize = 0;
		httpResponsePtr.Reset();
		error = "";
		// if(buffer != NULL){
		// 	delete buffer;
		// 	buffer = NULL;
		// }
	}
	~HTTPDownloader(){
		Clear();
	}
};

class FileSizePair {
	public:
	std::string filename = "";
	size_t size = 0;
	FileSizePair(std::string filename, size_t size){
		this->filename = filename;
		this->size = size;
	}
};

class RadarDownloaderTask : public AsyncTaskRunner{
public:
	// site to download from
	std::string siteId = "KMKX";
	// location to download from
	std::string httpServerPath = "https://nomads.ncep.noaa.gov/pub/data/nccf/radar/nexrad_level2/";
	// file to get list of files from
	std::string listFile = "dir.list";
	// the list file does not update nearly as often as the last file so downloading the last file decreases latency
	bool alwaysDownloadLastFile = true;
	// time between polling for downloads
	float pollInterval = 30;
	// last time poling was initiated
	double lastPollTime = 0;
	// maximum number of files that will be downloaded before the current one
	size_t maxPerviousFilesToDownload = 10;
	
	std::string outputPath;
	std::string httpPath;
	
	void Init(){
		std::string httpServerPathWithTrailingSlash;
		if(httpServerPath.size() > 0 && httpServerPath[httpServerPath.size()-1] != '/'){
			httpServerPathWithTrailingSlash = httpServerPath + "/";
		}else{
			httpServerPathWithTrailingSlash = httpServerPath;
		}
		outputPath = StringUtils::GetUserPath("Data/Realtime/" + siteId + "/");
		httpPath = httpServerPathWithTrailingSlash + siteId + "/";
	}
	
	void Task(){
		Init();
		// httpPath = "http://localhost:54808/pub/data/nccf/radar/nexrad_level2/" + siteId + "/";
		
		std::string httpPathLocal = httpPath;
		std::string outputPathLocal = outputPath;
		
		//fprintf(stderr, "%s\n", outputPath.c_str());
		SystemAPI::CreateDirectory(outputPathLocal);
		
		HTTPDownloader dirListDownloader;
		//dirListDownloader.outputFile = "/tmp/list";
		while(!canceled){
			bool trueBool = true;
			dirListDownloader.MakeRequest(httpPathLocal + listFile, &canceled);
			fprintf(stderr, "List request complete %i %s\n", dirListDownloader.success, dirListDownloader.error.c_str());
			lastPollTime = SystemAPI::CurrentTime();
			if(dirListDownloader.success && dirListDownloader.bufferSize > 10){
				std::vector<FileSizePair> files = {};
				char* buffer = (char*)dirListDownloader.buffer;
				buffer[dirListDownloader.bufferSize - 1] = 0;
				char* line = strtok(buffer, "\n");
				// loop through each line of the string
				while( line != NULL ) {
					std::string lineString = std::string(line);
					// remove carriage returns
					size_t endPos = lineString.find('\r');
					if(endPos == std::string::npos){
						endPos = lineString.size();
					}
					size_t splitPos = lineString.find(' ');
					if(splitPos > endPos){
						splitPos = endPos;
					}
					if(splitPos < endPos){
						std::string part1 = lineString.substr(0, splitPos);
						std::string part2 = lineString.substr(splitPos + 1, endPos);
						// fprintf(stderr, "%s | %s\n", part2.c_str(), part1.c_str());
						files.push_back(FileSizePair(part2, (size_t)std::stoll(part1)));
					}
					line = strtok(NULL, "\n");
				}
				
				int errorCount = 0;
				for(int64_t i = files.size() - 1; i >= std::max(0i64, (int64_t)files.size() - 1 - (int64_t)maxPerviousFilesToDownload); i--){
					FileSizePair* file = &files[i];
					std::string filePath = outputPathLocal + file->filename;
					size_t existingSize = SystemAPI::GetFileStats(filePath).size;
					
					if(existingSize < file->size || (alwaysDownloadLastFile && i == files.size() - 1)){
						if(i != files.size() - 1){
							// throttle downloading of past data
							SystemAPI::Sleep(3);
						}
						// download file
						HTTPDownloader fileDownloader;
						fileDownloader.outputFile = filePath;
						fileDownloader.firstByte = existingSize;
						fileDownloader.timeout = std::min(pollInterval, 600.0f);
						fileDownloader.MakeRequest(httpPathLocal + file->filename, &canceled);
						fprintf(stderr, "Download request complete %i %s\n", fileDownloader.success, fileDownloader.error.c_str());
						if(!fileDownloader.success){
							errorCount++;
							if(errorCount > 2){
								// give up for now if there are errors
								SystemAPI::Sleep(30);
								break;
							}
						}
					}
					if(lastPollTime + pollInterval < SystemAPI::CurrentTime() || canceled){
						// begin polling over again
						break;
					}
				}
			}else{
				if(canceled){
					break;
				}
				// failed to download list so wait 20 seconds
				for(int i = 0; i < 20 && !canceled; i++){
					SystemAPI::Sleep(1);
				}
				
			}
			
			
			if(canceled){
				break;
			}
			// wait for next poll interval
			while(lastPollTime + pollInterval >= SystemAPI::CurrentTime() && !canceled){
				SystemAPI::Sleep(1);
			}
			// SystemAPI::Sleep(std::max(0.0, lastPollTime + pollInterval - SystemAPI::CurrentTime()));
			
			
			// prevent an error from launching a denial of service attack
			// this should never actually be needed but is here just in case I screw up
			SystemAPI::Sleep(1);
		}
	}
};

// Sets default values
ARadarDataDownloader::ARadarDataDownloader()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARadarDataDownloader::BeginPlay()
{
	Super::BeginPlay();
	// downloaderTask = new RadarDownloaderTask();
	// downloaderTask->Start();
}

void ARadarDataDownloader::EndPlay(const EEndPlayReason::Type endPlayReason) {
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		// unregister all events
		for(auto id : callbackIds){
			globalState->UnregisterEvent(id);
		}
	}
	if(downloaderTask != NULL){
		downloaderTask->Delete();
		downloaderTask = NULL;
	}
	Super::EndPlay(endPlayReason);
}

// Called every frame
void ARadarDataDownloader::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {
		GlobalState* globalState = &gameState->globalState;
		if(globalState->downloadData == true){
			if(downloaderTask == NULL){
				downloaderTask = new RadarDownloaderTask();
			}
			if(downloaderTask->siteId != globalState->downloadSiteId || downloaderTask->httpServerPath != globalState->downloadUrl){
				downloaderTask->siteId = globalState->downloadSiteId;
				downloaderTask->httpServerPath = globalState->downloadUrl;
				// cancel and restart downloader
				downloaderTask->Cancel();
				fprintf(stderr, "Canceled downloader\n");
			}
			// limit to 10 or more seconds between requests
			downloaderTask->pollInterval = std::max(10.0f, globalState->downloadPollInterval);
			downloaderTask->maxPerviousFilesToDownload = (size_t)globalState->downloadPreviousCount;
			if(!downloaderTask->running){
				downloaderTask->Init();
				downloaderTask->Start();
				fprintf(stderr, "Starting downloader %i %i %i\n", downloaderTask->canceled, downloaderTask->running, downloaderTask->finished);
				// open directory in viewer
				globalState->EmitEvent("LoadDirectory", downloaderTask->outputPath, NULL);
			}
		}
		if(globalState->downloadData == false && downloaderTask != NULL){
			downloaderTask->Delete();
			downloaderTask = NULL;
		}
	}
}

