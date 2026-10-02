// NoteManager.cpp
#include "NoteManager.h"
#include "Engine/World.h"

ANoteManager::ANoteManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ANoteManager::InitNotes(const TArray<FNoteData>& InNotes)
{
    Notes = InNotes;
}

void ANoteManager::UpdateSpawn(float CurrentTime, float SpawnOffset)
{
    for (FNoteData& Note : Notes)
    {
        if (!Note.bSpawned && CurrentTime >= Note.Time - SpawnOffset)
        {
            SpawnNote(Note);
            Note.bSpawned = true;
        }
    }
}

void ANoteManager::SpawnNote(const FNoteData& Note)
{
    UClass* SpawnClass = nullptr;

    if (Note.Type == TEXT("tap"))   SpawnClass = *TapNoteBP;
    else if (Note.Type == TEXT("flick")) SpawnClass = *FlickNoteBP;
    else if (Note.Type == TEXT("hold"))  SpawnClass = *HoldNoteBP;

    if (!SpawnClass) return;

    FVector SpawnLocation = GetLanePosition(Note.Lane);

    GetWorld()->SpawnActor<AActor>(SpawnClass, SpawnLocation, FRotator::ZeroRotator);
}

FVector ANoteManager::GetLanePosition(int32 Lane) const
{
    // 仮のレーン配置（Y方向に並べる）
    return FVector(0.f, Lane * 200.f, 0.f);
}
