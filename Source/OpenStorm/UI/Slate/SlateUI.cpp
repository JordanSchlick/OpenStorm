#include "SlateUI.h"
#include "SlateUIResources.h"

#include "UObject/UObjectGlobals.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "SCompass.h"


USlateUI::USlateUI(){
	resources = CreateDefaultSubobject<USlateUIResources>(TEXT("SlateUIResources"));
	//NewObject<class USlateUIResources>(this);
}

void USlateUI::AddToViewport(UGameViewportClient* gameViewport) {
	SAssignNew(hudWidget, SOverlay);
	SAssignNew(compass, SCompass);

	hudWidget->AddSlot()[compass.ToSharedRef()].HAlign(HAlign_Right).VAlign(VAlign_Bottom);


	gameViewport->AddViewportWidgetContent(hudWidget.ToSharedRef(), 5);
}

USlateUI::~USlateUI(){
	if(hudWidget.IsValid()){
		// the dumb/smart way to remove element from the viewport
		auto overlay = StaticCastSharedPtr<SOverlay>(hudWidget->GetParentWidget());
		if (overlay.IsValid()) {
			overlay->RemoveSlot(hudWidget.ToSharedRef());
		}
			
		//gameViewport->RemoveViewportWidgetContent(hudWidget.ToSharedRef());
	}
}

void USlateUI::SetCompassRotation(float rotation) {
	if (compass.IsValid()) {
		compass->Rotate(rotation);
	}
}