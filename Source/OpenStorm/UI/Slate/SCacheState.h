#include "CoreMinimal.h"
//#include "Widgets/SOverlay.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateColorBrush.h"

#include <vector>


class SCacheState : public SConstraintCanvas {
public:
	float height = 15;
	float width = 400;
	float boarder = 0.0;
	int cellCount = 0;
	
	std::vector<TSharedPtr<SImage>> cells;
	TSharedPtr<SImage> selector;
	
	FSlateBrush selectorBrush =      FSlateColorBrush(FLinearColor(1.0, 1.0, 1.0, 0.5));
	FSlateColorBrush unloadedBrush = FSlateColorBrush(FLinearColor(0.1, 0.1, 0.1, 0.5));
	FSlateColorBrush loadingBrush =  FSlateColorBrush(FLinearColor(0.1, 0.1, 0.4, 0.5));
	FSlateColorBrush loadedBrush =   FSlateColorBrush(FLinearColor(0.1, 0.4, 0.1, 0.5));
	FSlateColorBrush errorBrush =    FSlateColorBrush(FLinearColor(0.6, 0.1, 0.1, 0.5));
	//TSharedPtr<UTexture2D> roseTexture;

	SCacheState();
	void Construct(const FArguments& inArgs);
	virtual FVector2D ComputeDesiredSize(float layoutScaleMultiplier) const override;
	//void Tick(float deltaTime);
	void UpdateState();
};