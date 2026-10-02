// NoteManager.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoteData.h"
#include "NoteManager.generated.h"

UCLASS()
class ANoteManager : public AActor
{
    GENERATED_BODY()

public:
    ANoteManager();

    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> TapNoteBP;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> FlickNoteBP;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> HoldNoteBP;

    void InitNotes(const TArray<FNoteData>& InNotes);
    void UpdateSpawn(float CurrentTime, float SpawnOffset);

protected:
    TArray<FNoteData> Notes;

    void SpawnNote(const FNoteData& Note);
    FVector GetLanePosition(int32 Lane) const;
};
