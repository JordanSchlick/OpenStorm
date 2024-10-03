#include "LocationMarker.h"


#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"
#include "../EngineHelpers/StringUtils.h"
#include "../Objects/RadarGameStateBase.h"

ALocationMarker::ALocationMarker()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	UStaticMesh* mesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'")).Object;
	meshMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/UnlitColor.UnlitColor'")).Object;
	meshComponent->SetStaticMesh(mesh);
	meshComponent->SetMaterial(0, meshMaterial);
	meshComponent->SetRelativeScale3D(FVector(0.05, 0.05, 0.05));
	
	textComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	textMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/UnlitColorText.UnlitColorText'")).Object;
	textComponent->SetTextMaterial(textMaterial);
	textComponent->SetHorizontalAlignment(EHTA_Center);
	textComponent->SetWorldSize(20);
	textComponent->SetRelativeRotation(FRotator(0, 180, 0));
	
	
	collisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Text Collision"));
	collisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	collisionComponent->SetActive(false);
	collisionComponent->SetBoxExtent(FVector(0, 0, 0));

	textComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	meshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	collisionComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

}


void ALocationMarker::BeginPlay() {
	PrimaryActorTick.bCanEverTick = false;
	Super::BeginPlay();
	//SetColor(FVector(0,1.0f,0));
}

void ALocationMarker::Tick(float DeltaTime) {
	// auto camera = Cast<UCameraComponent>(GetWorld()->GetFirstPlayerController()->GetPawn()->GetComponentByClass(UCameraComponent::StaticClass()));
	// if (camera != NULL) {
	// 	//FVector playerLocation = camera->GetComponentLocation();
	// 	//FVector selfLocation = GetActorLocation();
	// 	//FRotator rotation = FRotationMatrix::MakeFromX(playerLocation - selfLocation).Rotator();
	// 	//SetActorRotation(rotation);
	// 	SetActorRotation(camera->GetComponentRotation());
	// }
	
}

void ALocationMarker::SetText(std::string text) {
	textComponent->SetText(FText::FromString(StringUtils::STDStringToFString(text)));
}

void ALocationMarker::SetColor(FVector color) {
	// initialize material instances if not already done and the color is not white
	if(meshMaterialInstance == NULL && (color.X != 1 || color.Y != 1 || color.Z != 1)){
		meshMaterialInstance = UMaterialInstanceDynamic::Create(meshMaterial, this);
		textMaterialInstance = UMaterialInstanceDynamic::Create(textMaterial, this);
		meshComponent->SetMaterial(0, meshMaterialInstance);
		textComponent->SetTextMaterial(textMaterialInstance);
	}
	if(meshMaterialInstance != NULL){
		meshMaterialInstance->SetVectorParameterValue("Color", color);
		textMaterialInstance->SetVectorParameterValue("Color", color);
	}
}

void ALocationMarker::EnableCollision() {
	FBoxSphereBounds textBounds = textComponent->GetLocalBounds();
	collisionComponent->SetBoxExtent(textBounds.BoxExtent + 20);
	collisionComponent->SetRelativeLocation(-textBounds.Origin);
	collisionComponent->SetActive(true);
}

void ALocationMarker::OnClick(){
	if(markerType == MarkerTypeRadarSite){
		ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
		if(gameMode != NULL){
			GlobalState* globalState = &gameMode->globalState;
			fprintf(stderr, "Set radar site to %s\n", data.c_str());
			globalState->downloadSiteId = data;
			if(!globalState->downloadData){
				globalState->openDownloadDropdown = true;
			}
		}
	}
}