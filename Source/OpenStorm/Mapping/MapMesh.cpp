#include "MapMesh.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

#include "../Radar/SimpleVector3.h"
#include "../Radar/Globe.h"
#include "Data/ElevationData.h"
#include "MapMeshManager.h"

#include <cmath>

inline SimpleVector3<> FVectorToSimpleVector3(FVector vec){
	return SimpleVector3<>(vec.X, vec.Y, vec.Z);
}
inline SimpleVector3<> FVectorToSimpleVector3(FRotator vec){
	return SimpleVector3<>(vec.Roll, vec.Pitch, vec.Yaw);
}

AMapMesh::AMapMesh(){
	material = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/TestImageMaterial.TestImageMaterial'")).Object;
	proceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	proceduralMesh->bUseAsyncCooking = true;
	RootComponent = proceduralMesh;
}

void AMapMesh::BeginPlay(){
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
	PrimaryActorTick.bCanEverTick = false;
	Super::BeginPlay();
	UpdatePosition(SimpleVector3(), SimpleVector3());
	//GenerateMesh();
}

void AMapMesh::EndPlay(const EEndPlayReason::Type endPlayReason) {
	DestroyChildren();
}

void AMapMesh::Tick(float DeltaTime){
	//GenerateMesh();
	//ProjectionOntoGlobeTest();
}

void AMapMesh::GenerateMesh(){
	TArray<FVector> vertices = {};
	TArray<int> triangles = {};
	TArray<FVector2D> uv0 = {};
	vertices.SetNum((divisions + 1) * (divisions + 1));
	uv0.SetNum((divisions + 1) * (divisions + 1));
	triangles.Empty((divisions) * (divisions) * 6);
	
	
	for (int x = 0; x <= divisions; x++) {
		for (int y = 0; y <= divisions; y++) {
			int loc = y * (divisions + 1) + x;
			SimpleVector3<double> vert = SimpleVector3<double>(
				0,
				longitudeRadians + longitudeWidthRadians * (x / (float)divisions - 0.5f),
				latitudeRadians - latitudeHeightRadians * (y / (float)divisions - 0.5f)
			);
			vert.radius() = ElevationData::GetDataAtPointRadians(vert.phi(), vert.theta());
			vert = globe->GetPointScaled(vert);
			
			//fprintf(stderr, "loc %f %f %f\n", vert.x, vert.y, vert.z);
			vertices[loc] = FVector(vert.x, vert.y, vert.z);
			
			uv0[loc] = FVector2D(x / (float)divisions, y / (float)divisions);
		}
	}
	
	for (int x = 0; x < divisions; x++) {
		for (int y = 0; y < divisions; y++) {
			int sectionChildId = (x * 2 / divisions) + 2 * (y * 2 / divisions);
			if(mapChildren[sectionChildId] != NULL && mapChildren[sectionChildId]->loaded){
				// punch hole for child
				continue;
			}
			int loc1 = y * (divisions + 1) + x;
			int loc2 = y * (divisions + 1) + x + 1;
			int loc3 = (y + 1) * (divisions + 1) + x;
			int loc4 = (y + 1) * (divisions + 1) + x + 1;
			int triangleLoc = (y * (divisions + 1) + x) * 6;
			
			triangles.Add(loc1);
			triangles.Add(loc3);
			triangles.Add(loc2);
			
			triangles.Add(loc2);
			triangles.Add(loc3);
			triangles.Add(loc4);
		}
	}
	if(triangles.Num() == 0){
		SetActorHiddenInGame(true);
	}else{
		SetActorHiddenInGame(false);
		proceduralMesh->CreateMeshSection(0, vertices, triangles, TArray<FVector>(), uv0, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		proceduralMesh->SetMaterial(0, material);
	}
}

void AMapMesh::SetBounds(double latitudeRadiansIn, double longitudeRadiansIn, double latitudeHeightRadiansIn, double longitudeWidthRadiansIn){
	latitudeRadians = latitudeRadiansIn;
	longitudeRadians = longitudeRadiansIn;
	latitudeHeightRadians = latitudeHeightRadiansIn;
	longitudeWidthRadians = longitudeWidthRadiansIn;
	SimpleVector3<> corner1 = globe->GetPointScaled(latitudeRadians - latitudeHeightRadians / 2.0, longitudeRadians - longitudeWidthRadians / 2.0, 1);
	SimpleVector3<> corner2 = globe->GetPointScaled(latitudeRadians + latitudeHeightRadians / 2.0, longitudeRadians + longitudeWidthRadians / 2.0, 1);
	corner1.Subtract(corner2);
	subdivideDistance = corner1.Magnitude();
	if(layer < 2){
		subdivideDistance *= 4;
	}
	GenerateMesh();
}

void AMapMesh::MakeChildren(){
	if(manager != NULL){
		maxLayer = manager->maxLayer;
	}
	bool madeChildren = false;
	if(layer < maxLayer){
		for(int i = 0; i < 4; i++){
			if(mapChildren[i] == NULL){
				AMapMesh* child = GetWorld()->SpawnActor<AMapMesh>(AMapMesh::StaticClass());
				child->globe = globe;
				child->manager = manager;
				child->mapParent = this;
				child->childId = i;
				child->layer = layer + 1;
				child->SetBounds(
					((i == 1 || i == 3) ? 1 : -1) * latitudeHeightRadians * 0.25 + latitudeRadians,
					((i == 0 || i == 1) ? 1 : -1) * longitudeWidthRadians * 0.25 + longitudeRadians,
					latitudeHeightRadians / 2.0,
					longitudeWidthRadians / 2.0
				);
				child->UpdatePosition(FVectorToSimpleVector3(GetActorLocation()), appliedRotation);
				mapChildren[i] = child;
				madeChildren = true;
			}
		}
	}
	if(madeChildren){
		GenerateMesh();
	}
}

void AMapMesh::DestroyChildren(){
	bool destroyedChildren = false;
	for(int i = 0; i < 4; i++){
		if(mapChildren[i] != NULL){
			mapChildren[i]->Destroy();
			mapChildren[i] = NULL;
			destroyedChildren = true;
		}
	}
	if(destroyedChildren){
		GenerateMesh();
	}
}

void AMapMesh::Update(){
	int childCount = 0;
	for(int i = 0; i < 4; i++){
		if(mapChildren[i] != NULL){
			mapChildren[i]->Update();
			childCount++;
		}
	}
	if(manager != NULL){
		SimpleVector3<> position = centerPosition;
		position.Subtract(manager->cameraLocation);
		float distance = position.Magnitude();
		if(distance < subdivideDistance){
			//fprintf(stderr, "Yes %f/%f\n", distance, subdivideDistance);
			if(childCount < 4){
				MakeChildren();
			}
		}else{
			//fprintf(stderr, "No %f/%f\n", distance, subdivideDistance);
			if(childCount > 0){
				DestroyChildren();
			}
		}
	}
}

void AMapMesh::UpdatePosition(SimpleVector3<> position, SimpleVector3<> rotation){
	for(int i = 0; i < 4; i++){
		if(mapChildren[i] != NULL){
			mapChildren[i]->UpdatePosition(position, rotation);
		}
	}
	SetActorLocation(FVector(position.x, position.y, position.z));
	
	//SetActorRotation(FRotator(rotation.y, rotation.z, rotation.x));
	centerPosition = globe->GetPointScaled(latitudeRadians, longitudeRadians, ElevationData::GetDataAtPointRadians(latitudeRadians, longitudeRadians));
	centerPosition.RotateAroundZ(rotation.z);
	centerPosition.RotateAroundX(rotation.x);
	appliedRotation = rotation;
	SetActorRotation(FQuat(FVector(1, 0, 0), -rotation.x) * FQuat(FVector(0, 0, 1), -rotation.z));
	
	//centerPosition.RotateAroundY(rotation.y);
	centerPosition.Add(position);
}

void AMapMesh::ProjectionOntoGlobeTest(){
	TArray<FVector> vertices = {};
	TArray<int> triangles = {};
	TArray<FVector2D> uv0 = {};
	vertices.SetNum((divisions + 1) * (divisions + 1));
	uv0.SetNum((divisions + 1) * (divisions + 1));
	triangles.Empty((divisions) * (divisions) * 6);
	
	Globe globe2 = {};
	globe2.scale = 1.0 / 1000000.0 * 100.0;
	globe2.SetCenter(0, 0, -globe2.surfaceRadius);
	//globe.SetTopCoordinates(40, 0);
	
	
	FVector actorLocTmp = GetActorLocation();
	SimpleVector3<double> actorLoc = SimpleVector3<double>(actorLocTmp.X, actorLocTmp.Y, actorLocTmp.Z);
	for (int x = 0; x <= divisions; x++) {
		for (int y = 0; y <= divisions; y++) {
			int loc = y * (divisions + 1) + x;
			SimpleVector3<double> vert = SimpleVector3<double>(x * 10, y * 10, 0);
			vert.Add(actorLoc);
			vert = globe2.GetLocationScaled(vert);
			vert.radius() = 0;
			vert = globe2.GetPointScaled(vert);
			vert.Subtract(actorLoc);
			//fprintf(stderr, "loc %f %f %f\n", vert.x, vert.y, vert.z);
			vertices[loc] = FVector(vert.x, vert.y, vert.z);
			
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
			
			triangles.Add(loc1);
			triangles.Add(loc3);
			triangles.Add(loc2);
			
			triangles.Add(loc2);
			triangles.Add(loc3);
			triangles.Add(loc4);
		}
	}
	
	proceduralMesh->CreateMeshSection(0, vertices, triangles, TArray<FVector>(), uv0, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	proceduralMesh->SetMaterial(0, material);
}
