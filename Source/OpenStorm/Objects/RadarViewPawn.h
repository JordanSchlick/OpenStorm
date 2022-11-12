// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "RadarVolumeRender.h"
#include "../UI/ImGuiUI.h"
#include "../UI/Slate/SlateUI.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "RadarViewPawn.generated.h"

UCLASS()
class OPENSTORM_API ARadarViewPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARadarViewPawn();
	~ARadarViewPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	template <class T>
	T* FindActor();

private:
	float forwardMovement = 0;
	float sidewaysMovement = 0;
	float verticalMovement = 0;
	float verticalRotation = 0;
	float horizontalRotation = 0;
	float verticalRotationAmount = 0;
	float horizontalRotationAmount = 0;

	void MoveFB(float Value);
	void MoveLR(float Value);
	void MoveUD(float Value);
	void RotateLR(float Value);
	void RotateUD(float Value);
	void RotateMouseLR(float Value);
	void RotateMouseUD(float Value);
	void ReleaseMouse();
	void PressMouse();
	
private:
	UPROPERTY(EditAnywhere)
		float moveSpeed = 300.0f;
	UPROPERTY(EditAnywhere)
		float rotateSpeed = 200.0f;
	
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* meshComponent;
	
	UPROPERTY(EditAnywhere)
		ARadarVolumeRender* mainVolumeRender = NULL;

	UPROPERTY(EditAnywhere)
		AImGuiUI* gui = NULL;
	
	UPROPERTY(EditAnywhere)
		UMaterialInstanceDynamic* radarMaterialInstance = NULL;
	
	UPROPERTY(EditAnywhere)
		UCameraComponent * camera = NULL;

	UPROPERTY(EditAnywhere)
		ASlateUI* hud = NULL;
};
