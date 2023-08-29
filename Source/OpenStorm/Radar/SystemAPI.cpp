#include "SystemAPI.h"




#include <string>
#include <vector>
#include <chrono>

#include <string>
#include <locale>
#include <codecvt>

#include <sys/stat.h> // stat
#include <errno.h>    // errno, ENOENT, EEXIST
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>   // _mkdir
// #include <fileapi.h>
// #include <stringapiset.h>
#undef CreateDirectory
#else
#include <dirent.h>
#endif


inline bool isDirExist(const std::string& path){
#if defined(_WIN32)
    struct _stat info;
	std::string dirPath(path); 
	while ('\\' == *dirPath.rbegin()) dirPath.pop_back();
    if (_stat(dirPath.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & _S_IFDIR) != 0;
#else 
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
#endif
}

inline bool makePath(const std::string& path){
#if defined(_WIN32)
    int ret = _mkdir(path.c_str());
#else
    mode_t mode = 0755;
    int ret = mkdir(path.c_str(), mode);
#endif
    if (ret == 0)
        return true;

    switch (errno)
    {
    case ENOENT:
        // parent didn't exist, try to create it
        {
            int pos = path.find_last_of('/');
            if (pos == std::string::npos)
#if defined(_WIN32)
                pos = path.find_last_of('\\');
            if (pos == std::string::npos)
#endif
                return false;
            if (!makePath( path.substr(0, pos) ))
                return false;
        }
        // now, try to create again
#if defined(_WIN32)
        return 0 == _mkdir(path.c_str());
#else 
        return 0 == mkdir(path.c_str(), mode);
#endif

    case EEXIST:
        // done!
        return isDirExist(path);

    default:
        return false;
    }
}



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

// std::vector<std::string> SystemAPI::ReadDirectory(std::string path) {
// //#ifdef _WIN32
// 	//fprintf(stderr, "path %s\n", path.c_str());
// 	std::vector<std::string> files = {};
// 	TArray<FString> Files;
// 	FString FullPathFilename((path + "/*").c_str());
// 	IFileManager::Get().FindFiles(Files, *FullPathFilename, true, false);
// 	for (FString file : Files)
// 	{
// 		std::string fileString = std::string(TCHAR_TO_UTF8(*file));
// 		files.push_back(fileString);
// 	}
// 	return files;
// /*#else
// 	struct dirent* entry;
// 	DIR* dirFd = opendir(filePath.c_str());
// 	if (dirFd == NULL) {
// 		fprintf(stderr, "Directory not found");
// 		return;
// 	}
// 	while ((entry = readdir(dirFd)) != NULL) {
// 		fprintf(stderr, "%s\n", entry->d_name);
// 	}
// 	closedir(dirFd);
// #endif*/
// }

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

// bool SystemAPI::CreateDirectory(std::string path){
// 	FString FullPathFilename(path.c_str());
// 	return FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*FullPathFilename);
// }

#else

double SystemAPI::CurrentTime() {
	auto now = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration<double>(now).count();
}

// #ifdef _WIN32
// std::wstring s2ws(const std::string &s, bool isUtf8 = true)
// {
// 	int len;
// 	int slength = (int)s.length() + 1;
// 	len = MultiByteToWideChar(isUtf8 ? CP_UTF8 : CP_ACP, 0, s.c_str(), slength, 0, 0);
// 	std::wstring buf;
// 	buf.resize(len);
// 	MultiByteToWideChar(isUtf8 ? CP_UTF8 : CP_ACP, 0, s.c_str(), slength,
// 		const_cast<wchar_t *>(buf.c_str()), len);
// 	return buf;
// }
// #endif




SystemAPI::FileStats SystemAPI::GetFileStats(std::string path){
	SystemAPI::FileStats stats;
	// todo
	return stats;
}

#endif


bool SystemAPI::CreateDirectory(std::string path){
	return makePath(path);
}


std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

std::vector<SystemAPI::FileStats> SystemAPI::ReadDirectory(std::string path) {
	std::vector<FileStats> files;
#ifdef _WIN32
	// WIN32_FIND_DATAW findFileData;
	// HANDLE hFind;
	// hFind = FindFirstFileW(s2ws(path + "/*.*").c_str(), &findFileData);
	// wprintf(s2ws(path).c_str());
	// if (hFind != INVALID_HANDLE_VALUE) {
	// 	do {
	// 		files.push_back(ws2s(std::wstring(findFileData.cFileName)));
	// 	} while (FindNextFileW(hFind, &findFileData));
	// 	FindClose(hFind);
	// }else{
	// 	fprintf(stderr, "Directory not found\n");
	// }
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind;
	hFind = FindFirstFileExW(s2ws(path + "/*.*").c_str(), FindExInfoBasic, &findFileData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);;
	// wprintf(s2ws(path).c_str());
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			FileStats stats = {};
			stats.exists = true;
			stats.name = ws2s(std::wstring(findFileData.cFileName));
			ULARGE_INTEGER ul;
			ul.HighPart = findFileData.nFileSizeHigh;
			ul.LowPart = findFileData.nFileSizeLow;
			stats.size = ul.QuadPart;
			#define TICKS_PER_SECOND 10000000
			#define EPOCH_DIFFERENCE 11644473600LL
			stats.mtime = (*(long long*)&findFileData.ftLastWriteTime) / TICKS_PER_SECOND - EPOCH_DIFFERENCE;
			stats.isDirectory = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			files.push_back(stats);
		} while (FindNextFileW(hFind, &findFileData));
		FindClose(hFind);
	}else{
		fprintf(stderr, "Directory not found\n");
	}
#else
	struct dirent* entry;
	DIR* dirFd = opendir(filePath.c_str());
	if (dirFd == NULL) {
		fprintf(stderr, "Directory not found\n");
		return;
	}
	// should probably be optimized like https://stackoverflow.com/questions/58889085/faster-way-to-get-the-total-space-taken-by-the-directory-containing-5-million-fi
	while ((entry = readdir(dirFd)) != NULL) {
		// fprintf(stderr, "%s\n", entry->d_name);
		FileStats stats = {};
		stats.exists = true;
		stats.name = std::string(entry->d_name);
		stats.isDirectory = entry->d_type == DT_DIR;
		files.push_back(stats);
	}
	closedir(dirFd);
#endif
	return files;
}