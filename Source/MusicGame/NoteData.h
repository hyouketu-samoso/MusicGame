#pragma once

#include "CoreMinimal.h"
#include "NoteData.generated.h"

USTRUCT(BlueprintType)
struct FNoteData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Time = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Lane = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Type = TEXT("tap");

	UPROPERTY(Transient)
	bool bSpawned = false;
};


USTRUCT(BlueprintType)
struct FChartData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FNoteData> Notes;
};