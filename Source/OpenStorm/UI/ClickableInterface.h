// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ClickableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UClickableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Actors that use this interface will be clickable by the mouse
 */
class OPENSTORM_API IClickableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	enum ClickableType {
		ClickableTypeClick, // fires OnClick after the mouse is released
		// ClickableTypeDrag,
	};
	// when the raycast hit this function is called to determine what type of interaction will happen
	virtual ClickableType GetClickableType();
	// fired when the object is clicked
	virtual void OnClick();
};
