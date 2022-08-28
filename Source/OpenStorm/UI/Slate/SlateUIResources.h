#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/Texture2D.h"
#include "SlateUIResources.generated.h"

UCLASS()
class USlateUIResources : public UObject{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		UTexture2D* crosshairTexture;
		
	USlateUIResources();
	~USlateUIResources();
	
	static USlateUIResources* Instance;
private:
};