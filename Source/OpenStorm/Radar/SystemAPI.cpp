#include "SystemAPI.h"


//#include "Unix/UnixPlatformTime.h"
//#include "Windows/WindowsPlatformTime.h"
#include "GenericPlatform/GenericPlatformTime.h"

#include "CoreMinimal.h"
#include "HAL/FileManager.h"

#include <string>
#include <vector>

double SystemAPI::currentTime() {
	return FPlatformTime::Seconds();
}

std::vector<std::string> SystemAPI::readDirectory(std::string path) {
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