#pragma once


namespace NativeAPI{
	// allocate stderr and stdout and hide the console window
	void AllocateConsole();
	// show the console window
	void ShowConsole();
	
	// Use GetWorld()->GetGameViewport()->GetWindow()->GetDPIScaleFactor() instead
	float GetDisplayScale(void * hwnd);
}