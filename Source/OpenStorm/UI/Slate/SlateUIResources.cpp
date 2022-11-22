#include "SlateUIResources.h"
#include "UObject/ConstructorHelpers.h"

USlateUIResources::USlateUIResources(){
	crosshairTexture = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("Texture2D'/Game/Textures/crosshair.crosshair'")).Object;
	Instance = this;
}

USlateUIResources::~USlateUIResources(){
	Instance = NULL;
}

USlateUIResources* USlateUIResources::Instance = NULL;

/*
USlateUIResources* USlateUIResources::Get() {
	return Instance;
}*/
