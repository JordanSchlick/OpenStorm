#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "SlateUI.generated.h"



class SOverlay;
class SDPIScaler;
class SCompass;
class SCacheState;
class STextBlock;
class UGameViewportClient;
class USlateUIResources;

UCLASS()
class ASlateUI : public AActor{
	GENERATED_BODY()

public:
	TSharedPtr<SCompass> compass;
	TSharedPtr<SCacheState> cacheState;
	TSharedPtr<SOverlay> hudWidget;
	TSharedPtr<STextBlock> fileName;
	TSharedPtr<SDPIScaler> scaleWidget;
	ASlateUI();
	~ASlateUI();
	
	UPROPERTY(EditAnywhere)
		USlateUIResources* resources = NULL;
	
	void AddToViewport(UGameViewportClient* gameViewport);
	void SetCompassRotation(float rotation);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float DeltaTime) override;
};