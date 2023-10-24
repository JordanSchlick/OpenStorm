#include "SlateUI.h"
#include "SlateUIResources.h"

#include "../../Objects/RadarGameStateBase.h"

#include "UObject/UObjectGlobals.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Text/STextBlock.h"
#include "SCompass.h"
#include "SCacheState.h"


ASlateUI::ASlateUI(){
	resources = CreateDefaultSubobject<USlateUIResources>(TEXT("SlateUIResources"));
	//NewObject<class USlateUIResources>(this);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASlateUI::AddToViewport(UGameViewportClient* gameViewport) {
	SAssignNew(scaleWidget, SDPIScaler);
	SAssignNew(hudWidget, SOverlay);
	SAssignNew(compass, SCompass);
	SAssignNew(cacheState, SCacheState);

	hudWidget->AddSlot(2)[compass.ToSharedRef()].HAlign(HAlign_Right).VAlign(VAlign_Bottom);
	hudWidget->AddSlot(1)[cacheState.ToSharedRef()].HAlign(HAlign_Right).VAlign(VAlign_Bottom);


	scaleWidget->SetContent(hudWidget.ToSharedRef());
	gameViewport->AddViewportWidgetContent(scaleWidget.ToSharedRef(), 5);
}

ASlateUI::~ASlateUI(){
	
}

void ASlateUI::SetCompassRotation(float rotation) {
	if (compass.IsValid()) {
		compass->Rotate(rotation);
	}
}

void ASlateUI::BeginPlay() {
	Super::BeginPlay();
	AddToViewport(GetWorld()->GetGameViewport());
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	resources->globalState = globalState;
}

void ASlateUI::EndPlay(const EEndPlayReason::Type endPlayReason) {
	resources->globalState = NULL;
	if(hudWidget.IsValid()){
		// the dumb/smart way to remove element from the viewport
		auto overlay = StaticCastSharedPtr<SOverlay>(scaleWidget->GetParentWidget());
		if (overlay.IsValid()) {
			overlay->RemoveSlot(scaleWidget.ToSharedRef());
		}
			
		//gameViewport->RemoveViewportWidgetContent(hudWidget.ToSharedRef());
	}
}

void ASlateUI::Tick(float DeltaTime){
	// update scale based on system scalling
	if (scaleWidget.IsValid()) {
		SWindow* swindow = GetWorld()->GetGameViewport()->GetWindow().Get();
		//float nativeScale = swindow->GetDPIScaleFactor();
		if(resources->globalState->guiScale > 0){
			scaleWidget->SetDPIScale(resources->globalState->guiScale);
		}
	}
	
	if(cacheState.IsValid()){
		cacheState->UpdateState();
	}
	
	
}