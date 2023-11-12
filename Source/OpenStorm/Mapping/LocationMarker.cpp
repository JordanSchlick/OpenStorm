#include "LocationMarker.h"


#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
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
	UMaterial* material = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/TestImageMaterial.TestImageMaterial'")).Object;

	meshComponent->SetStaticMesh(mesh);
	meshComponent->SetMaterial(0, material);
	meshComponent->SetRelativeScale3D(FVector(0.05, 0.05, 0.05));
	
	textComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	textComponent->SetTextMaterial(ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Engine/EditorResources/FieldNodes/_Resources/DefaultTextMaterialOpaque.DefaultTextMaterialOpaque'")).Object);
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
void ALocationMarker::EnableCollision() {
	FBoxSphereBounds textBounds = textComponent->GetLocalBounds();
	collisionComponent->SetBoxExtent(textBounds.BoxExtent + 0);
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
			globalState->openDownloadDropdown = true;
		}
	}
}