#include "CoreMinimal.h"

class SWindow;
class UGameViewportClient;
class SWidget;
class SOverlay;

class UIWindow{
public:
	TSharedPtr<SWindow> window = NULL;

	TSharedPtr<SWidget> imGuiWidget = NULL;

	// viewport overlay of main window (not UIWindow)
	TSharedPtr<SOverlay> mainViewportOverlayWidget = NULL;

	bool isOpen = false;

	// create a new window and steal ImGui from viewport
	UIWindow(UGameViewportClient* viewport);

	~UIWindow();

	// gracefully close the window
	void Close();


	// return ImGui to main window. this is called automatically when Close is called.
	void ReturnImGui();
	void Tick();
};