// Copyright Void Interactive, 2021

#include "MoveToActivity.h"

UMoveToActivity::UMoveToActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "MoveTo");

	bAllowWhileArrested = true;
}

bool UMoveToActivity::CanShoot() const
{
	return true;
}
