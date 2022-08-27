#include "HUD.h"


#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "SCompass.h"


HUD::HUD(UGameViewportClient* gameViewport){
	SAssignNew(hudWidget, SOverlay);
	SAssignNew(compass, SCompass);
	
	hudWidget->AddSlot()[compass.ToSharedRef()].HAlign(HAlign_Right).VAlign(VAlign_Bottom);
	
	
	gameViewport->AddViewportWidgetContent(hudWidget.ToSharedRef(), 5);
}

HUD::~HUD(){
	if(hudWidget.IsValid()){
		// the dumb/smart way to remove element from the viewport
		auto overlay = StaticCastSharedPtr<SOverlay>(hudWidget->GetParentWidget());
		if (overlay.IsValid()) {
			overlay->RemoveSlot(hudWidget.ToSharedRef());
		}
			
		//gameViewport->RemoveViewportWidgetContent(hudWidget.ToSharedRef());
	}
}

void HUD::SetCompassRotation(float rotation) {
	if (compass.IsValid()) {
		compass->Rotate(rotation);
	}
}