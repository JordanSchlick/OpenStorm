// Fill out your copyright notice in the Description page of Project Settings.




#include "RadarVolumeRender.h"
#include "StdioConsole.h"


//#define VERBOSE 1
#include "../Radar/RadarData.h"
#include "../Radar/RadarColorIndex.h"


#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "Math/UnrealMathUtility.h"
#include "HAL/FileManager.h"

// modulo that always returns positive
inline int modulo(int i, int n) {
	return (i % n + n) % n;
}



// Sets default values
ARadarVolumeRender::ARadarVolumeRender()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a static mesh component
	cubeMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube"));

	// Set the component's mesh

	cubeMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'")).Object;

	storedMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/RadarVolumeMaterial.RadarVolumeMaterial'")).Object;



	cubeMeshComponent->SetStaticMesh(cubeMesh);

	cubeMeshComponent->SetMaterial(0, storedMaterial);

	// Set as root component
	RootComponent = cubeMeshComponent;

}



// Called when the game starts or when spawned
void ARadarVolumeRender::BeginPlay()
{
	Super::BeginPlay();

	// Load the Cube mesh
	cubeMeshComponent->SetStaticMesh(cubeMesh);

	AStdioConsole::ShowConsole();

	/*
	//IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ShowFlag.PostProcessing"));
	//CVar->Set(0);
	//GEngine->Exec(GetWorld(), TEXT("ShowFlag.PostProcessing 0"));
	GEngine->Exec(GetWorld(), TEXT("ShowFlag.ToneMapper 0"));

//#if UE_BUILD_SHIPPING
	GEngine->Exec(GetWorld(), TEXT("r.TonemapperGamma 0"));
	GEngine->Exec(GetWorld(), TEXT("r.TonemapperFilm 0"));
	GEngine->Exec(GetWorld(), TEXT("r.Tonemapper.Quality 0"));
	fprintf(stderr, "Disabled tonemapper in shipping\n");
//#endif
	*/

	//UE_LOG(LogTemp, Display, TEXT("%s"), StoredMaterial->GetName());

	radarMaterialInstance = UMaterialInstanceDynamic::Create(cubeMeshComponent->GetMaterial(0), this);
	cubeMeshComponent->SetMaterial(0, radarMaterialInstance);

	radarMaterialInstance->SetVectorParameterValue(TEXT("Center"), this->GetActorLocation());


	FString radarFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/Data/Demo/KMKX_20220723_235820"));
	FString fullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*radarFile);
	RadarData radarData = {};
	const char* fileLocaition = StringCast<ANSICHAR>(*fullPath).Get();
	radarData.ReadNexrad(fileLocaition);

	if (radarData.radiusBufferCount == 0) {
		fprintf(stderr, "Could not open %s\n", fileLocaition);
		return;
	}

	// PF_B8G8R8A8 - 4 uint8 values per pixel
	// PF_R32_FLOAT - single float value per pixel
	// PF_A32B32G32R32F - 4 float values per pixel

	volumeTexture = UTexture2D::CreateTransient(radarData.radiusBufferCount, (radarData.thetaBufferCount + 2) * radarData.sweepBufferCount, PF_R32_FLOAT);
	FTexture2DMipMap* MipMap = &(volumeTexture->PlatformData->Mips[0]);
	volumeImageData = &(MipMap->BulkData);
	radarMaterialInstance->SetTextureParameterValue(TEXT("Volume"), volumeTexture);


	// the angle index texture maps the angle from the ground to a sweep
	// pixel 65536 is strait up, pixel 32768 is parallel with the ground, pixel 0 is strait down
	// value 0 is no sweep, value 1.0-255.0 specify the index of the sweep, value 1.0 specifies first sweep, values in-between intagers will interpolate sweeps
	angleIndexTexture = UTexture2D::CreateTransient(256, 256, PF_R32_FLOAT);
	MipMap = &(angleIndexTexture->PlatformData->Mips[0]);
	angleIndexImageData = &(MipMap->BulkData);
	radarMaterialInstance->SetTextureParameterValue(TEXT("AngleIndex"), angleIndexTexture);
	

	valueIndexTexture = UTexture2D::CreateTransient(128, 128, PF_A32B32G32R32F);
	MipMap = &(valueIndexTexture->PlatformData->Mips[0]);
	valueIndexImageData = &(MipMap->BulkData);
	radarMaterialInstance->SetTextureParameterValue(TEXT("ValueIndex"), valueIndexTexture);


	

	radarMaterialInstance->SetScalarParameterValue(TEXT("InnerDistance"), radarData.innerDistance);

	RadarData::TextureBuffer imageBuffer = radarData.CreateTextureBufferReflectivity();
	float* RawImageData = (float*)volumeImageData->Lock(LOCK_READ_WRITE);
	memcpy(RawImageData, imageBuffer.data, imageBuffer.byteSize);
	volumeImageData->Unlock();
	volumeTexture->UpdateResource();

	delete[] imageBuffer.data;



	imageBuffer = radarData.CreateAngleIndexBuffer();

	float* rawAngleIndexImageData = (float*)angleIndexImageData->Lock(LOCK_READ_WRITE);
	memcpy(rawAngleIndexImageData, imageBuffer.data, imageBuffer.byteSize);
	angleIndexImageData->Unlock();
	angleIndexTexture->UpdateResource();

	delete[] imageBuffer.data;
	


	RadarColorIndexResult valueIndex = RadarColorIndex::relativeHue();

	float* rawValueIndexImageData = (float*)valueIndexImageData->Lock(LOCK_READ_WRITE);
	//memcpy(rawValueIndexImageData, valueIndex.data, 16384);
	
	memcpy(rawValueIndexImageData, valueIndex.data, valueIndex.byteSize);
	valueIndexImageData->Unlock();
	valueIndexTexture->UpdateResource();

	delete[] valueIndex.data;
	// set value bounds
	radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexLower"), valueIndex.lower);
	radarMaterialInstance->SetScalarParameterValue(TEXT("ValueIndexUpper"), valueIndex.upper);

	

	

	//RandomizeTexture();
}

static uint32_t deadbeef_seed;
static uint32_t deadbeef_beef = 0xdeadbeef;

inline uint32_t deadbeefRand() {
	deadbeef_seed = (deadbeef_seed << 7) ^ ((deadbeef_seed >> 25) + deadbeef_beef);
	deadbeef_beef = (deadbeef_beef << 7) ^ ((deadbeef_beef >> 25) + 0xdeadbeef);
	return deadbeef_seed;
}

// Called every frame
void ARadarVolumeRender::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//return;
	
}


void ARadarVolumeRender::RandomizeTexture() {

	uint8* RawImageData = (uint8*)volumeImageData->Lock(LOCK_READ_WRITE);
	float* RawImageDataFloat = (float*)RawImageData;

	uint8_t intensity = deadbeefRand();
	//RawImageData[0] = FMath::RandRange(0, 255);
	//*
	for (int x = 1; x < 128 * 128 * 8 * 4; x++) {
		//RawImageData[x] = FMath::RandRange(0, 255);
		RawImageData[x] = deadbeefRand();
		//RawImageData[x] = intensity;
	}
	for (int x = 0; x < 128 * 128 * 8 * 4; x+=4) {
		//RawImageData[x] = FMath::RandRange(0, 255);
		RawImageData[x+3] = 255;
		//RawImageData[x] = intensity;
	}
	//*/

	/*
	for (int x = 1; x < 128 * 128 * 8; x++) {
		//RawImageData[x] = FMath::RandRange(0, 255);
		RawImageDataFloat[x] = deadbeefRand() > 0xEFFFFFFF ? (float)deadbeefRand() / (float)0xFFFFFFFF : 0;
	}
	//*/

	volumeImageData->Unlock();
	volumeTexture->UpdateResource();
}

