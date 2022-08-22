#include "SystemAPI.h"




#include <string>
#include <vector>


#ifdef UE_GAME

//#include "Unix/UnixPlatformTime.h"
#ifdef PLATFORM_WINDOWS
#include "Windows/WindowsPlatformTime.h"
#endif

#include "GenericPlatform/GenericPlatformTime.h"


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
	return FPlatformTime::Seconds();
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

#endif