// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <vector>
#include "ImGuiController.generated.h"

class UIWindow;
class ImGuiUI;
class GlobalState;
namespace pfd{
	class public_open_file;
}

UCLASS()
class OPENSTORM_API AImGuiController : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AImGuiController();
	~AImGuiController();
	int unsafeFrames = 0;
	UIWindow* uiWindow = NULL;
	ImGuiUI* imGuiUI = NULL;
	pfd::public_open_file* fileChooser = NULL;
	std::vector<uint64_t> callbackIds = {};
	
	// if the left click is down and it has not been locked yet
	bool isLeftClicking = false;
	// actor selected by click that may be acted upon by further input
	AActor* selectedActor = NULL;
	
	// called when left clicking outside of UI
	void LeftClick();
	void LockMouse();
	void UnlockMouse();
	void InitializeConsole();
	// move gui to external window
	void ExternalWindow();
	// move gui to main viewport
	void InternalWindow();
	// set engine settings from global state
	void UpdateEngineSettings();
	// get reference to global state
	GlobalState* GetGlobalState();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};