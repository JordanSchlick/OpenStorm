// Fill out your copyright notice in the Description page of Project Settings.

/*
UI TODO

Variables to implement
GS->globalState.moveSpeed
GS->globalState.rotateSpeed
GS->globalState.fadeSpeed
GS->globalState.fade
GS->globalState.animate
GS->globalState.animateSpeed
GS->globalState.interpolation
GS->globalState.maxFPS

*/

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
	ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>();

	ImGui::Begin("Menu", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), 0, ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), 0);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	if (!ImGui::CollapsingHeader("Main")) {
		
		if (ImGui::TreeNode("Movement")) {
			ImGui::Text("Forward Movement Speed:");
			ImGui::SliderFloat("##1", &GS->globalState.moveSpeed, 0.0f, 1000.0f);
			
			ImGui::Text("Rotation Speed:");
			ImGui::SliderFloat("##2", &GS->globalState.rotateSpeed, 0.0f, 1000.0f);
			ImGui::TreePop();
			//ImGui::Text("MovementSpeed: %f", GS->globalState.moveSpeed);
		}
		ImGui::Separator();
		
		if (ImGui::TreeNode("Fade")) {
			ImGui::Checkbox("Fade", &GS->globalState.fade);
			ImGui::Text("Fade Speed:");
			ImGui::SliderFloat("##1", &GS->globalState.fadeSpeed, 0.0f, 5.0f);
			ImGui::TreePop();
		}
		ImGui::Separator();
		
		if (ImGui::TreeNode("Animation")) {
			ImGui::Checkbox("Animate", &GS->globalState.animate);
			ImGui::Checkbox("Interpolation", &GS->globalState.interpolation);
			ImGui::Text("Animation Speed:");
			ImGui::SliderFloat("##1", &GS->globalState.animateSpeed, 0.0f, 5.0f);
			ImGui::TreePop();
		}
		//ImGui::Separator();

	}	
	if (!ImGui::CollapsingHeader("Filter")) {
		ImGui::Text("TODO");
	}
	
	if (!ImGui::CollapsingHeader("Settings")) {
		ImGui::Text("Max FPS:");
		ImGui::InputInt("##1", &GS->globalState.maxFPS, 10);
	}
	
	if (ImGui::Button("Test")) {
		ligma(GS->globalState.inputToggle);
	}

	ImGui::End();
	
}

void AImGuiUI::ligma(bool Value)
{
	FImGuiModule::Get().GetProperties().SetInputEnabled(Value);

}

