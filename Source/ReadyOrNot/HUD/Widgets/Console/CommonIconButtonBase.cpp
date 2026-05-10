// Copyright Void Interactive, 2023


#include "CommonIconButtonBase.h"
#include "Input/UIActionBindingHandle.h"
#include "Input/CommonBoundActionButton.h"

bool UCommonIconButtonBase::Initialize()
{
	bool result = Super::Initialize();
	//BindingHandle = InputAction;
	BindTriggeringInputActionToClick();
	SetRepresentedAction(TriggeringBindingHandle);
	return result;
};