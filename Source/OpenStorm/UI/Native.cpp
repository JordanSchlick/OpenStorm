#include "Native.h"
#include "stdint.h"
#include "stdio.h"

#ifdef _WIN32
#define SW_HIDE 0
#define SW_SHOW 5
#define MONITOR_DEFAULTTONEAREST 2
typedef void* HANDLE;
typedef void* HINSTANCE;
typedef void* HWND;
typedef void* HMONITOR;
typedef long HRESULT;
typedef unsigned long DWORD;
typedef int BOOL;
extern "C" {
	__declspec(dllimport) HWND __stdcall GetConsoleWindow();
	__declspec(dllimport) BOOL __stdcall AllocConsole();
	__declspec(dllimport) HINSTANCE LoadLibraryA(const char* lpLibFileName);
	__declspec(dllimport) HINSTANCE LoadLibraryW(const wchar_t* lpLibFileName);
	__declspec(dllimport) BOOL FreeLibrary(HINSTANCE hLibModule);
	__declspec(dllimport) void* GetProcAddress(HINSTANCE hModule, const char* lpProcName);
	__declspec(dllimport) BOOL ShowWindow(HWND hWnd, int nCmdShow);
	typedef BOOL(__stdcall* ShowWindowDef)(HWND hWnd, int nCmdShow);
	typedef HRESULT(__stdcall* GetScaleFactorForMonitorDef)(HMONITOR hMon, int* pScale);
	typedef HMONITOR(__stdcall* MonitorFromWindowDef)(HWND hwnd, DWORD dwFlags);
}


#include "Microsoft/MinimalWindowsApi.h"
#endif


namespace NativeAPI{
	void AllocateConsole() {
#ifdef _WIN32
		HANDLE conHandleExisted = GetConsoleWindow();
		if (conHandleExisted == NULL)
		{
			AllocConsole();
		}
		HANDLE conHandle = GetConsoleWindow();
		if (conHandle != NULL)
		{
			freopen("CONOUT$", "w", stdout);
			freopen("CONOUT$", "w", stderr);
			//HINSTANCE hGetProcIDDLL = LoadLibraryW(L"user32.dll");
			/*auto hGetProcIDDLL = Windows::LoadLibraryW(L"User32.dll");
			if (!hGetProcIDDLL) {
				ShowWindowDef ShowWindow = (ShowWindowDef)GetProcAddress(hGetProcIDDLL, "ShowWindow");
				if (ShowWindow) {
					ShowWindow((HWND)conHandle, SW_HIDE);
				} else {
					fprintf(stderr, "ShowWindow load failed\n");
				}
				//FreeLibrary(hGetProcIDDLL);
				Windows::FreeLibrary(hGetProcIDDLL);
			} else {
				fprintf(stderr, "user32.dll load failed\n");
			}*/
			ShowWindow((HWND)conHandle, SW_HIDE);
		} else {
			fprintf(stderr, "No console handle found\n");
		}
#endif
	}


	void ShowConsole(){
	#ifdef __INTELLISENSE__
	#else
	#endif
	#ifdef _WIN32
		HANDLE conHandleExisted = GetConsoleWindow();
		if (conHandleExisted == NULL)
		{
			AllocConsole();
		}
		HANDLE conHandle = GetConsoleWindow();
		if (conHandle != NULL)
		{

			freopen("CONOUT$", "w", stdout);
			freopen("CONOUT$", "w", stderr);
			//HINSTANCE hGetProcIDDLL = LoadLibraryW(L"user32.dll");
			/*auto hGetProcIDDLL = Windows::LoadLibraryW(L"User32.dll");
			if (!hGetProcIDDLL) {
				ShowWindowDef ShowWindow = (ShowWindowDef)GetProcAddress(hGetProcIDDLL, "ShowWindow");
				if (ShowWindow) {
					ShowWindow((HWND)conHandle, SW_SHOW);
				} else {
					fprintf(stderr, "ShowWindow load failed\n");
				}
				//FreeLibrary(hGetProcIDDLL);
				Windows::FreeLibrary(hGetProcIDDLL);
			} else {
				fprintf(stderr, "user32.dll load failed\n");
			}*/
			ShowWindow((HWND)conHandle, SW_SHOW);
		} else {
			fprintf(stderr, "No console handle found");
		}
	#endif
	}
	
	
	float GetDisplayScale(void* hwnd){
		float scale = 1.0f;
#ifdef _WIN32
		HINSTANCE shcore = LoadLibraryA("Shcore.dll");
		HINSTANCE user32 = LoadLibraryA("user32.dll");
		if (user32 != NULL && shcore != NULL) {
			MonitorFromWindowDef MonitorFromWindow = (MonitorFromWindowDef)GetProcAddress(user32, "MonitorFromWindow");
			GetScaleFactorForMonitorDef GetScaleFactorForMonitor = (GetScaleFactorForMonitorDef)GetProcAddress(shcore, "GetScaleFactorForMonitor");
			if (GetScaleFactorForMonitor && MonitorFromWindow) {
				int scalePercent = 100;
				HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
				GetScaleFactorForMonitor(mon, &scalePercent);
				scale = scalePercent / 100.0f;
			} else {
				fprintf(stderr, "GetScaleFactorForMonitor load failed");
			}
		} else {
			fprintf(stderr, "user32.dll or Shcore.dll load failed");
		}
#endif
		return scale;
	}
	
}