// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImGuiUI.generated.h"


class UIWindow;
namespace pfd{
	class public_open_file;
}

UCLASS()
class OPENSTORM_API AImGuiUI : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AImGuiUI();
	~AImGuiUI();
	bool showDemoWindow = false;
	bool scalabilityTest = false;
	int unsafeFrames = 0;
	UIWindow* uiWindow = NULL;
	pfd::public_open_file* fileChooser = NULL;
	
	void LockMouse();
	void UnlockMouse();
	void InitializeConsole();
	// move gui to external window
	void ExternalWindow();
	// move gui to main viewport
	void InternalWindow();
	// choose a dierectory or specific files
	void ChooseFiles();
	// set engine settings from global state
	void UpdateEngineSettings();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ligma(bool Value);
};