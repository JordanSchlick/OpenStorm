#include "RadarSlice.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Modules/ModuleManager.h"
#include "Engine/World.h"

#include "../Radar/SimpleVector3.h"
#include "../Radar/AsyncTask.h"
#include "../Radar/Globe.h"
#include "../Application/GlobalState.h"
#include "./RadarVolumeRender.h"
#include "./RadarGameStateBase.h"

#include <cmath>
#include <algorithm>

#define M_PI       3.14159265358979323846
#define M_PIF       3.14159265358979323846f


template <class T>
T* ARadarSlice::FindActor() {
	TArray<AActor*> FoundActors = {};
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), T::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0) {
		return (T*)FoundActors[0];
	} else {
		return NULL;
	}
}


ARadarSlice::ARadarSlice(){
	proceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	proceduralMesh->bUseAsyncCooking = true;
	RootComponent = proceduralMesh;
}

void ARadarSlice::BeginPlay(){
	static Globe defaultGlobe = {};
	//defaultGlobe.scale = 1.0 / 10000.0 * 100.0;
	//defaultGlobe.SetCenter(0, 0, -defaultGlobe.surfaceRadius);
	//defaultGlobe.SetTopCoordinates(45, 0);
	//defaultGlobe.SetTopCoordinates(27.9879493, 86.9172718); //mt everest
	//defaultGlobe.SetTopCoordinates(63.06630592, -151.00722251); //mt Denali
	//defaultGlobe.SetTopCoordinates(45.8991878, -115.0331426); //mountains in america
	//defaultGlobe.SetTopCoordinates(42.967900, -88.550667);// KMKX
	if(globe == NULL){
		globe = &defaultGlobe;
	}
	PrimaryActorTick.bCanEverTick = true;
	mainVolumeRender = FindActor<ARadarVolumeRender>();
	Super::BeginPlay();
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode == NULL){
		return;
	}
	GlobalState* globalState = &gameMode->globalState;
	
	callbackIds.push_back(globalState->RegisterEvent("GlobeUpdate",[this](std::string stringData, void* extraData){
		GenerateMesh();
	}));
	callbackIds.push_back(globalState->RegisterEvent("VolumeUpdate",[this](std::string stringData, void* extraData){
		RadarData* radarData = (RadarData*)extraData;
		radius = radarData->stats.boundRadius * radarData->stats.pixelSize / 10000.0f * 100.0f * 1.1f;
		GenerateMesh();
	}));
	
	GenerateMesh();
}

void ARadarSlice::EndPlay(const EEndPlayReason::Type endPlayReason) {
	Super::EndPlay(endPlayReason);
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode == NULL){
		return;
	}
	GlobalState* globalState = &gameMode->globalState;
	for(auto id : callbackIds){
		globalState->UnregisterEvent(id);
	}
	callbackIds.clear();
}

void ARadarSlice::Tick(float DeltaTime){
	//GenerateMesh();
	//ProjectionOntoGlobeTest();
	if (radarMaterialInstance == NULL) {
		if (mainVolumeRender != NULL) {
			if (mainVolumeRender->radarMaterialInstance != NULL) {
				radarMaterialInstance = mainVolumeRender->radarMaterialInstance;
				proceduralMesh->SetMaterial(0, radarMaterialInstance);
			}
		}
	}
	
	ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
	if(gameMode == NULL){
		return;
	}
	GlobalState* globalState = &gameMode->globalState;
	globe = globalState->globe;
	
	
	if(globalState->viewMode == GlobalState::VIEW_MODE_SLICE){
		bool regenerateMesh = false;
		
		if(globalState->sliceAltitude != sliceAltitude){
			sliceAltitude = globalState->sliceAltitude;
			regenerateMesh = true;
		}
		if(globalState->sliceAngle != sliceAngle){
			sliceAngle = globalState->sliceAngle;
			regenerateMesh = true;
		}
		if(globalState->sliceMode != sliceMode){
			sliceMode = globalState->sliceMode;
			regenerateMesh = true;
		}
		
		
		if(regenerateMesh){
			GenerateMesh();
		}
		SetActorHiddenInGame(false);
	}else{
		SetActorHiddenInGame(true);
	}
}


void ARadarSlice::GenerateMesh(){
	TArray<FVector> vertices = {};
	TArray<FVector> normals = {};
	TArray<int> triangles = {};
	TArray<FVector2D> uv0 = {};
	if(sliceMode == GlobalState::SLICE_MODE_CONSTANT_ALTITUDE){
		divisions = 30;
		vertices.SetNum((divisions + 1) * (divisions + 1));
		normals.SetNum((divisions + 1) * (divisions + 1));
		uv0.SetNum((divisions + 1) * (divisions + 1));
		triangles.Empty((divisions) * (divisions) * 6 * 2);
		
		// SimpleVector3<float> testVec = SimpleVector3<float>(1,2,3);
		// testVec.RectangularToSpherical();
		// testVec.SphericalToRectangular();
		// fprintf(stderr, "test %f %f %f \n", testVec.x, testVec.y, testVec.z);
		
		for (int x = 0; x <= divisions; x++) {
			for (int y = 0; y <= divisions; y++) {
				int loc = y * (divisions + 1) + x;
				SimpleVector3<double> vert = SimpleVector3<double>(
					(x / (float)divisions - 0.5f) * radius * 2.0f,
					(y / (float)divisions - 0.5f) * radius * 2.0f,
					0
				);
				
				vert = globe->GetLocationScaled(vert);
				vert.radius() = sliceAltitude;
				vert = globe->GetPointScaled(vert);
				
				//fprintf(stderr, "loc %f %f %f\n", vert.x, vert.y, vert.z);
				vertices[loc] = FVector(vert.x, vert.y, vert.z);
				
				vert.Subtract(globe->center);
				vert.Multiply(1.0f / vert.Magnitude());
				normals[loc] = FVector(vert.x, vert.y, vert.z);
				
				uv0[loc] = FVector2D(x / (float)divisions, y / (float)divisions);
			}
		}
		
		for (int x = 0; x < divisions; x++) {
			for (int y = 0; y < divisions; y++) {
				int loc1 = y * (divisions + 1) + x;
				int loc2 = y * (divisions + 1) + x + 1;
				int loc3 = (y + 1) * (divisions + 1) + x;
				int loc4 = (y + 1) * (divisions + 1) + x + 1;
				int triangleLoc = (y * (divisions + 1) + x) * 6;
				
				// first triangle of quad
				triangles.Add(loc1);
				triangles.Add(loc3);
				triangles.Add(loc2);
				// first triangle of quad backside
				triangles.Add(loc2);
				triangles.Add(loc3);
				triangles.Add(loc1);
				
				// second triangle of quad
				triangles.Add(loc2);
				triangles.Add(loc3);
				triangles.Add(loc4);
				// second triangle of quad backside
				triangles.Add(loc4);
				triangles.Add(loc3);
				triangles.Add(loc2);
			}
		}
	}
	if(sliceMode == GlobalState::SLICE_MODE_SWEEP_ANGLE){
		vertices.SetNum(sectors + 1);
		normals.SetNum(sectors + 1);
		uv0.SetNum(sectors + 1);
		triangles.Empty((sectors) * 3 * 2);
		
		vertices[0] = FVector(0, 0, 0);
		uv0[0] = FVector2D(0.5f, 0.5f);
		normals[0] = FVector(0, 0, 1);
		
		for (int i = 0; i < sectors; i++) {
			SimpleVector3<float> vert = SimpleVector3<float>(radius, 0, 0);
			vert.RotateAroundY(-sliceAngle / 180.0f * M_PIF);
			vert.RotateAroundZ(i / (float)sectors * 2.0f * M_PIF);
			
			vertices[i + 1] = FVector(vert.x, vert.y, vert.z);
			uv0[i + 1] = FVector2D(vert.x / radius + 0.5, vert.y / radius + 0.5);
			normals[i + 1] = FVector(0, 0, 1);
		}
		for (int i = 0; i < sectors; i++) {
			// front triangle
			triangles.Add(i + 1);
			triangles.Add(i + 2);
			triangles.Add(0);
			// back triangle
			triangles.Add(0);
			triangles.Add(i + 2);
			triangles.Add(i + 1);
		}
		// add last triangle front
		triangles.Add(sectors + 1);
		triangles.Add(1);
		triangles.Add(0);
		// add last triangle back
		triangles.Add(0);
		triangles.Add(1);
		triangles.Add(sectors + 1);
	}
	proceduralMesh->CreateMeshSection(0, vertices, triangles, normals, uv0, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
}