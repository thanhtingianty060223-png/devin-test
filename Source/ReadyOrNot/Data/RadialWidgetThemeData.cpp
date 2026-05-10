// Copyright Void Interactive, 2021

#include "RadialWidgetThemeData.h"

URadialWidgetThemeData::URadialWidgetThemeData()
{
	IFMODStudioModule::Get();
	ConstructorHelpers::FObjectFinderOptional<UFMODEvent> RM_Select(TEXT("FMODEvent'/Game/FMOD/Events/UI/Radial_Menu/RM_Select.RM_Select'"));
	ConstructorHelpers::FObjectFinderOptional<UFMODEvent> RM_Open(TEXT("FMODEvent'/Game/FMOD/Events/UI/Radial_Menu/RM_Open.RM_Open'"));
	ConstructorHelpers::FObjectFinderOptional<UFMODEvent> RM_Close(TEXT("FMODEvent'/Game/FMOD/Events/UI/Radial_Menu/RM_Close.RM_Close'"));
	ConstructorHelpers::FObjectFinderOptional<UFMODEvent> RM_CloseNoSelection(TEXT("FMODEvent'/Game/FMOD/Events/UI/Radial_Menu/RM_Collapse.RM_Collapse'"));
	
#if !UE_SERVER
	static ConstructorHelpers::FObjectFinder<UFont> LouisGeorgeBold(TEXT("Font'/Game/ReadyOrNot/Assets/Font/Louis_George_Cafe_Bold_Font.Louis_George_Cafe_Bold_Font'"));
	Font = LouisGeorgeBold.Object;
#endif
	
	
	SelectionSound = RM_Select.Get();
	MenuOpenSound = RM_Open.Get();
	MenuCloseSound = RM_Close.Get();
	MenuCloseSound_NoSelection = RM_CloseNoSelection.Get();


	
	bHideRadialWheelCursorOnMenuOpened = true;
}
