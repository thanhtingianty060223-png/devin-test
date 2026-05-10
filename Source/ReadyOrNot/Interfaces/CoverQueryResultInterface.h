// Void Interactive, 2020

#pragma once

#include "Info/Activities/Tasks/FindCoverTask.h"
#include "UObject/Interface.h"
#include "CoverQueryResultInterface.generated.h"

UINTERFACE(MinimalAPI)
class UCoverQueryResultInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class READYORNOT_API ICoverQueryResultInterface
{
	GENERATED_BODY()

public:
	virtual const FCoverData* GetCoverData() { return nullptr; }
	virtual const FFindCoverQuery* GetCoverQuery() { return nullptr; }
	virtual bool ShouldDrawDebugLabels() const { return true; }
	virtual bool ShouldDrawScore() const { return true; }
	virtual bool ShouldDrawFailReason() const { return true; }
	virtual bool ShouldDrawPass() const { return true; }
	virtual bool ShouldDrawFail() const { return true; }
};
