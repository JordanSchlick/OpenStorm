#include "SystemAPI.h"




#include <string>
#include <vector>
#include <chrono>


#ifdef UE_GAME

//#include "Unix/UnixPlatformTime.h"
#ifdef PLATFORM_WINDOWS
//#include "Windows/WindowsPlatformTime.h"
#include "Windows/WindowsPlatformFile.h"
#endif

//#include "GenericPlatform/GenericPlatformTime.h"
#include "GenericPlatform/GenericPlatformFile.h"


#include "CoreMinimal.h"
#include "HAL/FileManager.h"
//void __cdecl std::_Xbad_function_call() {
	// for #include <functional>
	// this symbol is missing in unreal for some reason
//}
//void __cdecl std::_Xlength_error(char const*) {
	// for #include <map>
	// this symbol is missing in unreal for some reason
//}


#ifdef _WIN32
#endif


double SystemAPI::CurrentTime() {
	//return FPlatformTime::Seconds();
	FDateTime now = FDateTime::UtcNow();
	//fprintf(stderr," %f %f\n", (now.GetTicks() % 10000000) / 10000000.0, now.GetMillisecond() / 1000.0);
	//return (double)now.ToUnixTimestamp() + now.GetMillisecond() / 1000.0;
	return (double)now.ToUnixTimestamp() + (now.GetTicks() % 10000000) / 10000000.0;
}

std::vector<std::string> SystemAPI::ReadDirectory(std::string path) {
//#ifdef _WIN32
	//fprintf(stderr, "path %s\n", path.c_str());
	std::vector<std::string> files = {};
	TArray<FString> Files;
	FString FullPathFilename((path + "/*").c_str());
	IFileManager::Get().FindFiles(Files, *FullPathFilename, true, false);
	for (FString file : Files)
	{
		std::string fileString = std::string(TCHAR_TO_UTF8(*file));
		files.push_back(fileString);
	}
	return files;
/*#else
	struct dirent* entry;
	DIR* dirFd = opendir(filePath.c_str());
	if (dirFd == NULL) {
		fprintf(stderr, "Directory not found");
		return;
	}
	while ((entry = readdir(dirFd)) != NULL) {
		fprintf(stderr, "%s\n", entry->d_name);
	}
	closedir(dirFd);
#endif*/
}

SystemAPI::FileStats SystemAPI::GetFileStats(std::string path){
	FileStats stats = {};
	FString FullPathFilename(path.c_str());
	FFileStatData statData = IFileManager::Get().GetStatData(*FullPathFilename);
	if(!statData.bIsValid){
		return stats;
	}
	stats.exists = true;
	stats.isDirectory = statData.bIsDirectory;
	stats.size = (statData.FileSize == -1) ? 0 : statData.FileSize;
	stats.mtime = (double)statData.ModificationTime.ToUnixTimestamp() + statData.ModificationTime.GetMillisecond() / 1000.0;
	return stats;
}

#else

double SystemAPI::CurrentTime() {
	auto now = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration<double>(now).count();
}


std::vector<std::string> SystemAPI::ReadDirectory(std::string path) {
	std::vector<std::string> files;
	// todo
	return files;
}


SystemAPI::FileStats SystemAPI::GetFileStats(std::string path){
	SystemAPI::FileStats stats;
	// todo
	return stats;
}
#endif