// Fill out your copyright notice in the Description page of Project Settings.





//#ifdef __INTELLISENSE__
// visual studio intelisense is stupid, this keeps windows headers out of the intellisense shit show
//#else
//#ifdef _WIN32
//#define _AMD64_
//#define _WIN32_WINNT_WIN10_RS1 0
//#define WIN32_LEAN_AND_MEAN 
//#include <consoleapi3.h>
//#include <ConsoleApi.h>
//#include <libloaderapi.h>
//#undef WIN32_LEAN_AND_MEAN
//#endif
//#endif

#include "StdioConsole.h"

#ifdef _WIN32
#define SW_SHOW 5
typedef void* HANDLE;
typedef void* HINSTANCE;
typedef void* HWND;
typedef int BOOL;
extern "C" {
	__declspec(dllimport) HWND __stdcall GetConsoleWindow();
	__declspec(dllimport) BOOL __stdcall AllocConsole();
	__declspec(dllimport) HINSTANCE LoadLibraryA(const char* lpLibFileName);
	__declspec(dllimport) void* GetProcAddress(HINSTANCE hModule, const char* lpProcName);
	typedef BOOL(__stdcall* ShowWindowDef)(HWND hWnd, int nCmdShow);
}
#endif





#include "Components/StaticMeshComponent.h"

// Sets default values
AStdioConsole::AStdioConsole()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	UStaticMeshComponent* meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	RootComponent = meshComponent;
}




// Called when the game starts or when spawned
void AStdioConsole::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AStdioConsole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AStdioConsole::ShowConsole(){
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
		HINSTANCE hGetProcIDDLL = LoadLibraryA("user32.dll");
		if (hGetProcIDDLL != NULL) {
			ShowWindowDef ShowWindow = (ShowWindowDef)GetProcAddress(hGetProcIDDLL, "ShowWindow");
			if (ShowWindow) {
				ShowWindow((HWND)conHandle, SW_SHOW);
			} else {
				UE_LOG(LogTemp, Display, TEXT("ShowWindow load failed"));
			}
		} else {
			UE_LOG(LogTemp, Display, TEXT("user32.dll load failed"));
		}
		//ShowWindow((HWND)conHandle, SW_SHOW);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		//freopen("CONIN$", "w", stdin);
	} else {
		UE_LOG(LogTemp, Display, TEXT("No console handle gound"));
	}
#endif
}

/*
static int pfd[2];
static pthread_t thr;
static const char* tag = "myapp";

int start_logger(const char* app_name)
{
	tag = app_name;

	// make stdout line-buffered and stderr unbuffered
	setvbuf(stdout, 0, _IOLBF, 0);
	setvbuf(stderr, 0, _IONBF, 0);

	// create the pipe and redirect stdout and stderr
	pipe(pfd);
	dup2(pfd[1], 1);
	dup2(pfd[1], 2);

	// spawn the logging thread
	if (pthread_create(&thr, 0, thread_func, 0) == -1)
		return -1;
	pthread_detach(thr);
	return 0;
}

static void* thread_func(void*)
{
	ssize_t rdsz;
	char buf[128];
	while ((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
		if (buf[rdsz - 1] == '\n') --rdsz;
		buf[rdsz] = 0;  // add null-terminator
		__android_log_write(ANDROID_LOG_DEBUG, tag, buf);
	}
	return 0;
}
*/