#include "MapMesh.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Modules/ModuleManager.h"

#include "../Radar/SimpleVector3.h"
#include "../Radar/AsyncTask.h"
#include "../Radar/SystemAPI.h"
#include "../Radar/Globe.h"
#include "Data/ElevationData.h"
#include "Data/TileProvider.h"
#include "MapMeshManager.h"

#include <cmath>
#include <algorithm>

#define M_PI       3.14159265358979323846

inline SimpleVector3<> FVectorToSimpleVector3(FVector vec){
	return SimpleVector3<>(vec.X, vec.Y, vec.Z);
}
inline SimpleVector3<> FVectorToSimpleVector3(FRotator vec){
	return SimpleVector3<>(vec.Roll, vec.Pitch, vec.Yaw);
}

inline int LonToTileX(double lon, int z) 
{ 
	return (int)(floor((lon + M_PI) / (M_PI * 2.0) * (1 << z))); 
}

inline double LonToTileXDecimal(double lon, int z) 
{ 
	return (lon + M_PI) / (M_PI * 2.0) * (1 << z); 
}

inline int Lat2TileY(double lat, int z)
{ 
	return (int)(floor((1.0 - asinh(tan(lat)) / M_PI) / 2.0 * (1 << z))); 
}

inline double Lat2TileYDecimal(double lat, int z)
{ 
	return (1.0 - asinh(tan(lat)) / M_PI) / 2.0 * (1 << z); 
}

inline double TileXToLon(int x, int z) 
{
	return x / (double)(1 << z) * (M_PI * 2.0) - M_PI;
}

inline double TileYToLat(int y, int z) 
{
	double n = M_PI - 2.0 * M_PI * y / (double)(1 << z);
	return atan(0.5 * (exp(n) - exp(-n)));
}



// load tile from memory into a texture
class TextureLoader : public AsyncTaskRunner {
public:
	AMapMesh* mapMesh;
	Tile* tile;
	TextureLoader(AMapMesh* mapMesh, Tile* tile) {
		this->mapMesh = mapMesh;
		this->tile = tile;
	}
	
	UTexture2D* CreateTexture(UObject* Outer, const TArray64<uint8>& PixelData, int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, FName BaseName)
	{
		// based on UTexture2D::CreateTransient with a few modifications
		if (InSizeX <= 0 || InSizeY <= 0 ||
			(InSizeX % GPixelFormats[InFormat].BlockSizeX) != 0 ||
			(InSizeY % GPixelFormats[InFormat].BlockSizeY) != 0)
		{
			return nullptr;
		}
		
		while(IsGarbageCollectingAndLockingUObjectHashTables()){
			// wait for garbage collection to finish because objects can not be created during it
			SystemAPI::Sleep(0.01);
		}
		
		FName TextureName = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), BaseName);
		UTexture2D* NewTexture = NewObject<UTexture2D>(GetTransientPackage(), TextureName, RF_Transient);

		
		NewTexture->SetPlatformData(new FTexturePlatformData());
		NewTexture->GetPlatformData()->SizeX = InSizeX;
		NewTexture->GetPlatformData()->SizeY = InSizeY;
		NewTexture->GetPlatformData()->SetNumSlices(1);
		NewTexture->GetPlatformData()->PixelFormat = InFormat;
		
		// Allocate first mipmap and upload the pixel data
		int32 NumBlocksX = InSizeX / GPixelFormats[InFormat].BlockSizeX;
		int32 NumBlocksY = InSizeY / GPixelFormats[InFormat].BlockSizeY;
		FTexture2DMipMap* Mip = new FTexture2DMipMap();
		NewTexture->GetPlatformData()->Mips.Add(Mip);
		Mip->SizeX = InSizeX;
		Mip->SizeY = InSizeY;
		Mip->SizeZ = 1;
		Mip->BulkData.Lock(LOCK_READ_WRITE);
		void* TextureData = Mip->BulkData.Realloc((int64)NumBlocksX * NumBlocksY * GPixelFormats[InFormat].BlockBytes);
		FMemory::Memcpy(TextureData, PixelData.GetData(), PixelData.Num());
		Mip->BulkData.Unlock();

		NewTexture->UpdateResource();
		return NewTexture;
	}
	void Task() {
		//fprintf(stderr, "TextureLoader::Task");
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		EImageFormat imageFormat = ImageWrapperModule.DetectImageFormat(tile->data, tile->dataSize);
		if (imageFormat == EImageFormat::Invalid || canceled) {
			delete tile;
			return;
		}

		// Create an image wrapper for the detected image format
		TSharedPtr<IImageWrapper> imageWrapper = ImageWrapperModule.CreateImageWrapper(imageFormat);
		if (!imageWrapper.IsValid() || canceled) {
			delete tile;
			return;
		}

		TArray64<uint8> rawData;
		imageWrapper->SetCompressed(tile->data, tile->dataSize);
		imageWrapper->GetRaw(ERGBFormat::BGRA, 8, rawData);
		if (rawData.Num() == 0 || canceled) {
			delete tile;
			return;
		}

		delete tile;

		//FName TextureName = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(),  TEXT("TileTexture"));
		//UTexture2D* texture = UTexture2D::CreateTransient(imageWrapper->GetWidth(), imageWrapper->GetHeight(), EPixelFormat::PF_B8G8R8A8, TextureName);
		//texture->UpdateResource();
		UTexture2D* texture = CreateTexture(mapMesh, rawData, imageWrapper->GetWidth(), imageWrapper->GetHeight(), EPixelFormat::PF_B8G8R8A8,TEXT("TileTexture"));
		// texture->ConditionalBeginDestroy();
		
		if (canceled) {
			return;
		}
		// fprintf(stderr, "  %i %i  ", (int)rawData.Num(), (int)texture->GetSizeX());
		// uint8* buffer = rawData.GetData();
		// size_t bufferSizeBytes = rawData.Num();
		// int pixelSizeBytes = 4;
		// int width = texture->GetSizeX();
		// FUpdateTextureRegion2D* regions = new FUpdateTextureRegion2D[1]();
		// regions[0].DestX = 0;
		// regions[0].DestY = 0;
		// regions[0].SrcX = 0;
		// regions[0].SrcY = 0;
		// regions[0].Width = width;
		// regions[0].Height = std::min(bufferSizeBytes / pixelSizeBytes / width, (size_t)texture->GetSizeY());

		// texture->UpdateTextureRegions(0, 1, regions, width * pixelSizeBytes, pixelSizeBytes, (uint8*)buffer, [this, texture](uint8* dataPtr, const FUpdateTextureRegion2D* regionsPtr) {
		// 	delete regionsPtr;
		// });
		mapMesh->texture = texture;
		mapMesh->pendingTexture = true;
	}
};


// load tile into memory
class TileLoader : public AsyncTaskRunner {
public:
	AMapMesh* mapMesh;
	TileLoader(AMapMesh* mapMesh) {
		this->mapMesh = mapMesh;
	}
	void Task() {
		//fprintf(stderr, "TileLoader::Task");
		Tile* tile = mapMesh->manager->tileProvider->GetTile(mapMesh->layer, mapMesh->tileY, mapMesh->tileX);
		tile->SetCallback([this, tile]() {
			//fprintf(stderr, "TileLoader::Task::SetCallback");
			if (tile->data != NULL && !canceled) {
				mapMesh->textureLoader = new TextureLoader(mapMesh, tile);
				mapMesh->textureLoader->Start();
			} else {
				delete tile;
			}
		});
	}
};


AMapMesh::AMapMesh(){
	material = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/Map.Map'")).Object;
	materialInstance = UMaterialInstanceDynamic::Create(material, this, "MapMeshMaterialInstance");
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
	if(tileLoader != NULL){
		tileLoader->Cancel();
		tileLoader->Delete();
		tileLoader = NULL;
	}
	if(textureLoader != NULL){
		textureLoader->Cancel();
		textureLoader->Delete();
		textureLoader = NULL;
	}
	if(texture != NULL){
		texture->ConditionalBeginDestroy();
		// This mostly fixes a major memory leak in created textures
		//delete texture->GetPlatformData();
		//texture->SetPlatformData(NULL);
		//texture->GetPlatformData()->Mips.RemoveAt(0);
		// Ideally the entire platform data would be freed but that can occasionally cause crashes so we instead settle for freeing the largest part that won't crash
		texture->GetPlatformData()->Mips[0].BulkData.RemoveBulkData();
		texture = NULL;
	}
	Super::EndPlay(endPlayReason);
}

void AMapMesh::Tick(float DeltaTime){
	//GenerateMesh();
	//ProjectionOntoGlobeTest();
}

void AMapMesh::GenerateMesh(){
	TArray<FVector> vertices = {};
	TArray<FVector> normals = {};
	TArray<int> triangles = {};
	TArray<FVector2D> uv0 = {};
	vertices.SetNum((divisions + 1) * (divisions + 1));
	normals.SetNum((divisions + 1) * (divisions + 1));
	uv0.SetNum((divisions + 1) * (divisions + 1));
	triangles.Empty((divisions) * (divisions) * 6);
	
	// fully loaded object that owns the texture for this mesh
	AMapMesh* textureFromMesh = this;
	
	
	double tileLat = TileYToLat(tileY, layer);
	double tileLon = TileXToLon(tileX, layer);
	for (int x = 0; x <= divisions; x++) {
		for (int y = 0; y <= divisions; y++) {
			int loc = y * (divisions + 1) + x;
			SimpleVector3<double> vert = SimpleVector3<double>(
				0,
				longitudeRadians + longitudeWidthRadians * (x / (float)divisions - 0.5f) * 1.01,
				latitudeRadians - latitudeHeightRadians * (y / (float)divisions - 0.5f) * 1.01
			);
			
			
			//uv0[loc] = FVector2D(fmod(LonToTileXDecimal(vert.y, layer), 1.0), fmod(Lat2TileYDecimal(vert.z, layer), 1.0));
			uv0[loc] = FVector2D(
				LonToTileXDecimal(vert.y, layer) - tileX, 
				Lat2TileYDecimal(vert.z, layer) - tileY
			);
			
			
			vert.radius() = ElevationData::GetDataAtPointRadians(vert.phi(), vert.theta());
			vert = globe->GetPointScaled(vert);
			
			//fprintf(stderr, "loc %f %f %f\n", vert.x, vert.y, vert.z);
			vertices[loc] = FVector(vert.x, vert.y, vert.z);
			
			// does not take into account elevation changes
			// could be refactored to generate true normals
			vert.Multiply(1.0f / vert.Magnitude());
			normals[loc] = FVector(vert.x, vert.y, vert.z);
			
			//uv0[loc] = FVector2D(x / (float)divisions, y / (float)divisions);
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
		proceduralMesh->CreateMeshSection(0, vertices, triangles, normals, uv0, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		proceduralMesh->SetMaterial(0, materialInstance);
	}
}

void AMapMesh::LoadTile(){
	tileLoader = new TileLoader(this);
	tileLoader->Start();
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
	// ids
	// |0|1|
	// |2|3|
	
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
				child->tileX = tileX * 2 + ((i == 1 || i == 3) ? 1 : 0);
				child->tileY = tileY * 2 + ((i == 2 || i == 3) ? 1 : 0);
				double tileLon = TileXToLon(child->tileX, child->layer);
				double tileLat = TileYToLat(child->tileY, child->layer);
				double tileLonWidth = TileXToLon(child->tileX + 1, child->layer) - tileLon;
				double tileLatHeight = tileLat - TileYToLat(child->tileY + 1, child->layer);
				// child->SetBounds(
				// 	((i == 0 || i == 1) ? 1 : -1) * latitudeHeightRadians * 0.25 + latitudeRadians,
				// 	((i == 1 || i == 3) ? 1 : -1) * longitudeWidthRadians * 0.25 + longitudeRadians,
				// 	latitudeHeightRadians / 2.0,
				// 	longitudeWidthRadians / 2.0
				// );
				child->SetBounds(
					tileLat - tileLatHeight / 2.0,
					tileLon + tileLonWidth / 2.0,
					tileLatHeight,
					tileLonWidth
				);
				child->UpdatePosition(FVectorToSimpleVector3(GetActorLocation()), appliedRotation);
				child->UpdateTexture(materialInstance->K2_GetTextureParameterValue(TEXT("Texture")), true);
				child->UpdateParameters();
				child->LoadTile();
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
		if(distance < subdivideDistance || layer < manager->minLayer){
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
	if (pendingTexture) {
		// set newly loaded texture
		pendingTexture = false;
		fullyLoaded = true;
		UpdateTexture(texture, false);
	}
}

void AMapMesh::UpdateParameters(){
	if(manager != NULL){
		materialInstance->SetScalarParameterValue(TEXT("Brightness"), manager->mapBrightness);
	}
	for(int i = 0; i < 4; i++){
		if(mapChildren[i] != NULL){
			mapChildren[i]->UpdateParameters();
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

void AMapMesh::UpdateTexture(UTexture* newTexture, bool fromParent){
	materialInstance->SetTextureParameterValue(TEXT("Texture"), newTexture);
	if(fromParent && mapParent != NULL){
		FLinearColor offsetAndScale = mapParent->materialInstance->K2_GetVectorParameterValue(TEXT("OffsetAndScale"));
		// double parentTileLon = longitudeRadians - longitudeWidthRadians / 2.0;
		// double parentTileLat = latitudeRadians + latitudeHeightRadians / 2.0;
		// offsetAndScale.R = offsetAndScale.R + ((tileLon - parentTileLon) / longitudeWidthRadians) * offsetAndScale.B;
		// offsetAndScale.G = offsetAndScale.G + ((parentTileLat - tileLat) / latitudeHeightRadians) * offsetAndScale.A;
		// offsetAndScale.B *= tileLonWidth / longitudeWidthRadians;
		// offsetAndScale.A *= tileLatHeight / latitudeHeightRadians;
		offsetAndScale.R = offsetAndScale.R + ((childId == 1 || childId == 3) ? 1 : 0) * offsetAndScale.B * 0.5;
		offsetAndScale.G = offsetAndScale.G + ((childId == 2 || childId == 3) ? 1 : 0) * offsetAndScale.A * 0.5;
		offsetAndScale.B *= 0.5;
		offsetAndScale.A *= 0.5;
		materialInstance->SetVectorParameterValue(TEXT("OffsetAndScale"), offsetAndScale);
	}else{
		FLinearColor offsetAndScale = FLinearColor(0, 0, 1, 1);
		materialInstance->SetVectorParameterValue(TEXT("OffsetAndScale"), offsetAndScale);
	}
	for(int i = 0; i < 4; i++){
		if(mapChildren[i] != NULL && !mapChildren[i]->fullyLoaded){
			mapChildren[i]->UpdateTexture(newTexture, true);
		}
	}
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
