// Fill out your copyright notice in the Description page of Project Settings.


#include "ClickableInterface.h"

// Add default functionality here for any IClickableInterface functions that are not pure virtual.

IClickableInterface::ClickableType IClickableInterface::GetClickableType(){
	return ClickableTypeClick;
}

void IClickableInterface::OnClick(){
	
}
