#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SlateUI.generated.h"



class SOverlay;
class SCompass;
class UGameViewportClient;
class USlateUIResources;

UCLASS()
class USlateUI : public UObject{
	GENERATED_BODY()

public:
	TSharedPtr<SCompass> compass;
	TSharedPtr<SOverlay> hudWidget;
	USlateUI();
	~USlateUI();
	
	UPROPERTY(EditAnywhere)
		USlateUIResources* resources = NULL;
	
	void AddToViewport(UGameViewportClient* gameViewport);
	void SetCompassRotation(float rotation);
};