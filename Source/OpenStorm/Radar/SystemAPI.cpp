#include "SystemAPI.h"


//#include "Unix/UnixPlatformTime.h"
//#include "Windows/WindowsPlatformTime.h"
#include "GenericPlatform/GenericPlatformTime.h"

double SystemAPI::currentTime() {
	return FPlatformTime::Seconds();
}
