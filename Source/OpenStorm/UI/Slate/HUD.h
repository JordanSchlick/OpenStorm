#include "CoreMinimal.h"




class SOverlay;
class SCompass;

class HUD{
public:
	TSharedPtr<SCompass> compass;
	TSharedPtr<SOverlay> hudWidget;
	HUD(UGameViewportClient* gameViewport);
	~HUD();
	void SetCompassRotation(float rotation);
};