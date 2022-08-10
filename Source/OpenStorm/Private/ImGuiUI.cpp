// Fill out your copyright notice in the Description page of Project Settings.


#include "ImGuiUI.h"
#include <imgui.h>
#include <ImGuiModule.h>
#include "RadarGameStateBase.h"
#include <RadarViewPawn.h>

// Sets default values
AImGuiUI::AImGuiUI()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AImGuiUI::BeginPlay()
{
	Super::BeginPlay();
	
}

//const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());

// Called every frame
void AImGuiUI::Tick(float deltaTime)
{
	Super::Tick(deltaTime);
	static float Scale = 1.0f;
	ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>();

	ImGui::Begin("Menu", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), 0, ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), 0);
	ImGui::SliderFloat("float", &Scale, 0.0f, 5.0f);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("MovementSpeed: %f", GS->globalState.testFloat);
	
	
	if (ImGui::Button("Test")) {
		ligma(GS->globalState.inputToggle);
	}


	ImGui::End();
	
}

void AImGuiUI::ligma(bool Value)
{
	FImGuiModule::Get().GetProperties().SetInputEnabled(Value);

}

