// Copyright Void Interactive, 2023

#include "ProgressionData.h"

bool UProgressionRequirement::IsLocked(const TSet<FName>& InProgressionTags)
{
	return false;
}

bool ULevelCompleteRequirement::IsLocked(const TSet<FName>& InProgressionTags)
{
	FString GradeString;
	switch (RequiredGrade)
	{
	case ELevelGrade::S: GradeString = "s"; break;
	case ELevelGrade::APlus: GradeString = "aplus"; break;
	case ELevelGrade::A: GradeString = "a"; break;
	case ELevelGrade::B: GradeString = "b"; break;
	case ELevelGrade::C: GradeString = "c"; break;
	case ELevelGrade::D: GradeString = "d"; break;
	case ELevelGrade::F: GradeString = "f"; break;
	default: GradeString = "none";
	};
	
	FString RequiredTag = FString::Printf(TEXT("%s_grade_%s"), *RequiredLevel.ToString(), *GradeString);
	return !InProgressionTags.Contains(FName(RequiredTag));
}

TArray<FName> ULevelCompleteRequirement::GetLevelOptions() const
{
	UDataTable* LevelDataTable = UBpGameplayHelperLib::GetLevelLookupDataTable();
	if (!LevelDataTable)
		return {};

	TArray<FName> LevelOptions;
	for (auto& Element : LevelDataTable->GetRowMap())
	{
		FLevelDataLookupTable* Row = reinterpret_cast<FLevelDataLookupTable*>(Element.Value);
		if (!Row || Row->ProgressionTagPrefix.IsNone())
			continue;

		LevelOptions.Add(Row->ProgressionTagPrefix);
	}

	// ##UE5UPGRADE##
	LevelOptions.Sort([](const FName& a1, const FName& a2) {
		return a1.FastLess(a2);
	});;
	return LevelOptions;
}
