#include "CoreMinimal.h"

class SWindow;

class UIWindow{
public:
	TSharedPtr<SWindow> window = NULL;

	TSharedPtr<SWidget> imGuiWidget = NULL;

	// viewport overlay of main window (not UIWindow)
	TSharedPtr<SOverlay> mainViewportOverlayWidget = NULL;
	UGameViewportClient* mainViewport;

	bool isOpen = false;

	// create a new window and steel ImGui from viewport
	UIWindow(UGameViewportClient* viewport);

	~UIWindow();

	// gracefully close the window
	void Close();


	// return ImGui to main windo. this is called automatcaly when Close is called.
	void ReturnImGui();
	void Tick();
};