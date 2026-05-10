// Copyright Void Interactive, 2021

#pragma once

#include "BaseActivity.h"
#include "MoveToActivity.generated.h"

UCLASS()
class READYORNOT_API UMoveToActivity final : public UBaseActivity
{
	GENERATED_BODY()

public:
	UMoveToActivity();

	virtual bool CanShoot() const override;
};
