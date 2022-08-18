#pragma once


namespace NativeAPI{
	void ShowConsole();
	
	// Use GetWorld()->GetGameViewport()->GetWindow()->GetDPIScaleFactor() instead
	float GetDisplayScale(void * hwnd);
}