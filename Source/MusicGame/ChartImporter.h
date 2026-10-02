#pragma once
#include "CoreMinimal.h"
#include "NoteData.h"
#include "ChartImporter.generated.h"

UCLASS()
class UChartImporter : public UObject
{
    GENERATED_BODY()

public:
    // JSONì«Ç›çûÇ›ä÷êî
    bool LoadChart(const FString& FilePath, TArray<FNoteData>& OutNotes);
};
