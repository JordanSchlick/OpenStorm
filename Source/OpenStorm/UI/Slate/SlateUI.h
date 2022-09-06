#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "SlateUI.generated.h"



class SOverlay;
class SDPIScaler;
class SCompass;
class UGameViewportClient;
class USlateUIResources;

UCLASS()
class ASlateUI : public AActor{
	GENERATED_BODY()

public:
	TSharedPtr<SCompass> compass;
	TSharedPtr<SOverlay> hudWidget;
	TSharedPtr<SDPIScaler> scaleWidget;
	ASlateUI();
	~ASlateUI();
	
	UPROPERTY(EditAnywhere)
		USlateUIResources* resources = NULL;
	
	void AddToViewport(UGameViewportClient* gameViewport);
	void SetCompassRotation(float rotation);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
};