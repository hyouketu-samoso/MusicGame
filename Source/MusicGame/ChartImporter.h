#pragma once

#include "CoreMinimal.h"
#include "NoteData.h"
#include "ChartImporter.generated.h"

UCLASS()
class MUSICGAME_API UChartImporter : public UObject
{
	GENERATED_BODY()

public:

	bool LoadChart(
		const FString& FilePath,
		TArray<FNoteData>& OutNotes
	);
};