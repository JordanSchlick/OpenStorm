#include "CoreMinimal.h"
//#include "Widgets/SOverlay.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"



class SCompass : public SConstraintCanvas {
public:
	float lastRotation = -1;
	float size = 100;
	TSharedPtr<STextBlock> northText;
	TSharedPtr<STextBlock> southText;
	TSharedPtr<STextBlock> eastText;
	TSharedPtr<STextBlock> westText;
	TSharedPtr<SImage> rose;

	void Construct(const FArguments& inArgs);
	virtual FVector2D ComputeDesiredSize(float layoutScaleMultiplier) const override;
	void Rotate(float rotation);
};