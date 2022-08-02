// Fill out your copyright notice in the Description page of Project Settings.


#include "TestTexture.h"
#include "Engine/Texture2D.h"
#include "UObject/Object.h"
#include "Math/UnrealMathUtility.h"

// Sets default values
ATestTexture::ATestTexture()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a static mesh component
	cubeMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube"));

	// Set the component's mesh
	
	cubeMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'")).Object;

	StoredMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/TextureTestMaterial.TextureTestMaterial'")).Object;

	

	cubeMeshComponent->SetStaticMesh(cubeMesh);

	cubeMeshComponent->SetMaterial(0, StoredMaterial);

	// Set as root component
	RootComponent = cubeMeshComponent;

}

// Called when the game starts or when spawned
void ATestTexture::BeginPlay()
{
	Super::BeginPlay();
	
	// Load the Cube mesh
	cubeMeshComponent->SetStaticMesh(cubeMesh);


	

	//UE_LOG(LogTemp, Display, TEXT("%s"), StoredMaterial->GetName());

	DynamicMaterialInst = UMaterialInstanceDynamic::Create(cubeMeshComponent->GetMaterial(0) , cubeMesh);
	cubeMeshComponent->SetMaterial(0, DynamicMaterialInst);


	CustomTexture = UTexture2D::CreateTransient(1024, 1024);
	FTexture2DMipMap* MipMap = &(CustomTexture->PlatformData->Mips[0]);
	ImageData = &(MipMap->BulkData);
	

	/*UMaterialInstanceDynamic* DynamicMaterial = cubeMeshComponent->CreateDynamicMaterialInstance(0, cubeMeshComponent->GetMaterial(0));
	DynamicMaterial->SetTextureParameterValue("Texture", CustomTexture);
	cubeMeshComponent->SetMaterial(0, DynamicMaterial);*/
	DynamicMaterialInst->SetTextureParameterValue("Texture", CustomTexture);

	//UE_LOG(LogTemp, Display, TEXT("================"));

	ATestTexture::RandomizeTexture();
}

static uint32_t deadbeef_seed;
static uint32_t deadbeef_beef = 0xdeadbeef;

inline uint32_t deadbeefRand() {
	deadbeef_seed = (deadbeef_seed << 7) ^ ((deadbeef_seed >> 25) + deadbeef_beef);
	deadbeef_beef = (deadbeef_beef << 7) ^ ((deadbeef_beef >> 25) + 0xdeadbeef);
	return deadbeef_seed;
}

// Called every frame
void ATestTexture::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//return;
	
}

void ATestTexture::RandomizeTexture() {
	uint8* RawImageData = (uint8*)ImageData->Lock(LOCK_READ_WRITE);

	uint8_t intensity = deadbeefRand();
	//RawImageData[0] = FMath::RandRange(0, 255);
	for (int x = 1; x < 1024 * 1024 * 4; x++) {
		//RawImageData[x] = FMath::RandRange(0, 255);
		RawImageData[x] = deadbeefRand();
		//RawImageData[x] = intensity;
	}

	ImageData->Unlock();
	CustomTexture->UpdateResource();
}
