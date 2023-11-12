// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include <vector>

class AImGuiController;
namespace pfd{
	class public_open_file;
}

class ImGuiUI
{

public:
	// Sets default values for this actor's properties
	ImGuiUI();
	~ImGuiUI();
	
	bool showDemoWindow = false;
	bool scalabilityTest = false;
	AImGuiController* imGuiController = NULL;
	
	pfd::public_open_file* fileChooser = NULL;
	
	// choose a directory or specific files
	void ChooseFiles();


public:
	// main settings UI
	void MainUI();

	void ligma(bool Value);
};