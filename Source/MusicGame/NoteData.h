#pragma once
#include "CoreMinimal.h"
#include "NoteData.generated.h"

USTRUCT(BlueprintType)
struct FNoteData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    float Time;

    UPROPERTY(BlueprintReadWrite)
    int32 Lane;

    UPROPERTY(BlueprintReadWrite)
    FString Type;

    UPROPERTY(BlueprintReadWrite)
    float Duration;

    bool bSpawned = false;
};
