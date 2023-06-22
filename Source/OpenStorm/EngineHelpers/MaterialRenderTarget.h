#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Materials/Material.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "MaterialRenderTarget.generated.h"

UCLASS()
class UMaterialRenderTarget : public UCanvasRenderTarget2D {

	GENERATED_BODY()

public:
	UMaterialInterface* renderMaterial;
	
	UMaterialRenderTarget();
	
	

	static UMaterialRenderTarget* Create(int width, int height, EPixelFormat pixelFormat, UMaterialInterface* material, UObject* parent, const char* name = "");

	void Initialize(int width, int height, EPixelFormat pixelFormat);

	void Update();

	UFUNCTION()
	void DrawMaterial(UCanvas* canvas, int32 width, int32 height);
	//virtual void ReceiveUpdate(UCanvas* Canvas, int32 Width, int32 Height);

	// virtual uint32 CalcTextureMemorySizeEnum(ETextureMipCount Enum) const override;
	// why the fuck is this not defined properly in engine code?
	inline	uint32 CalcTextureMemorySizeEnum(ETextureMipCount Enum) const {
		// Calculate size based on format.  All mips are resident on render targets so we always return the same value.
		EPixelFormat Format = GetFormat();
		int32 BlockSizeX = GPixelFormats[Format].BlockSizeX;
		int32 BlockSizeY = GPixelFormats[Format].BlockSizeY;
		int32 BlockBytes = GPixelFormats[Format].BlockBytes;
		int32 NumBlocksX = (SizeX + BlockSizeX - 1) / BlockSizeX;
		int32 NumBlocksY = (SizeY + BlockSizeY - 1) / BlockSizeY;
		int32 NumBytes = NumBlocksX * NumBlocksY * BlockBytes;
		return NumBytes;
	}
};