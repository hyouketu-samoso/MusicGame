#include "SoundNoteSpawner.h"

#include "SoundNoteActor.h"
#include "HoldNoteActor.h"

#include "TimerManager.h"
#include "Engine/World.h"


ASoundNoteSpawner::ASoundNoteSpawner()
{
    PrimaryActorTick.bCanEverTick = false;

    // 4レーン
    LaneColors.SetNum(4);
}


void ASoundNoteSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoSpawn)
    {
        GetWorldTimerManager().SetTimer(
            AutoSpawnTimer,
            this,
            &ASoundNoteSpawner::SpawnWave,
            SpawnInterval,
            true
        );
    }
}


void ASoundNoteSpawner::SpawnWave()
{
    for (
        int32 LaneIndex = 0;
        LaneIndex < 4;
        ++LaneIndex
        )
    {
        SpawnNote(LaneIndex);
    }
}


void ASoundNoteSpawner::SpawnNote(
    int32 LaneIndex
)
{
    if (!LaneColors.IsValidIndex(LaneIndex))
    {
        return;
    }

    FNoteData TestNote;

    TestNote.Lane = LaneIndex;
    TestNote.Type = TEXT("tap");
    TestNote.Duration = 0.0f;

    SpawnNoteFromData(TestNote);
}


ASoundNoteActor*
ASoundNoteSpawner::SpawnNoteFromData(
    const FNoteData& NoteData
)
{
    if (!GetWorld())
    {
        return nullptr;
    }


    // ========================================
    // レーンチェック
    // ========================================

    if (!LaneColors.IsValidIndex(NoteData.Lane))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "NoteSpawner: Invalid LaneIndex = %d"
            ),
            NoteData.Lane
        );

        return nullptr;
    }


    // ========================================
    // ノーツタイプからクラスを決定
    // ========================================

    TSubclassOf<ASoundNoteActor> SpawnClass =
        GetNoteClassForType(
            NoteData.Type
        );

    if (!SpawnClass)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "NoteSpawner: No class for note type = %s"
            ),
            *NoteData.Type
        );

        return nullptr;
    }


    // ========================================
    // 判定ライン位置
    // ========================================

    const FVector TargetLocation =
        GetLaneTarget(
            NoteData.Lane
        );


    // ========================================
    // 出現位置
    // ========================================

    FVector SpawnLocation =
        TargetLocation +
        GetActorForwardVector() *
        SpawnDistance;

    SpawnLocation.Z += SpawnHeight;


    // ========================================
    // Spawn設定
    // ========================================

    FActorSpawnParameters Params;

    Params.Owner = this;

    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


    // ========================================
    // Actor生成
    // ========================================

    ASoundNoteActor* Note =
        GetWorld()->SpawnActor<ASoundNoteActor>(
            SpawnClass,
            SpawnLocation,
            FRotator::ZeroRotator,
            Params
        );

    if (!Note)
    {
        return nullptr;
    }


    // ========================================
    // ホールド設定
    // ========================================

    if (AHoldNoteActor* HoldNote =
        Cast<AHoldNoteActor>(Note))
    {
        HoldNote->SetHoldDuration(
            NoteData.Duration
        );
    }


    // ========================================
    // ノーツを飛ばす
    // ========================================

    Note->Launch(
        SpawnLocation,
        TargetLocation,
        ArcHeight,
        TravelTime,
        LaneColors[NoteData.Lane]
    );


    // ========================================
    // Spawn成功
    // ========================================

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "NoteSpawner: Spawned Note Lane=%d Type=%s"
        ),
        NoteData.Lane,
        *NoteData.Type
    );

    return Note;
}


TSubclassOf<ASoundNoteActor>
ASoundNoteSpawner::GetNoteClassForType(
    const FString& Type
) const
{
    if (
        Type.Equals(
            TEXT("tap"),
            ESearchCase::IgnoreCase
        )
        )
    {
        if (TapNoteClass)
        {
            return TapNoteClass;
        }
    }


    if (
        Type.Equals(
            TEXT("flick"),
            ESearchCase::IgnoreCase
        )
        )
    {
        if (FlickNoteClass)
        {
            return FlickNoteClass;
        }
    }


    if (
        Type.Equals(
            TEXT("hold"),
            ESearchCase::IgnoreCase
        )
        )
    {
        if (HoldNoteClass)
        {
            return HoldNoteClass;
        }
    }


    // 専用クラスがない場合
    // 共通クラスを使用

    return NoteClass;
}


FVector ASoundNoteSpawner::GetLaneTarget(
    int32 LaneIndex
) const
{
    const float Offset =
        (
            LaneIndex -
            (LaneColors.Num() - 1) * 0.5f
            ) *
        LaneSpacing;

    return
        GetActorLocation() +
        GetActorRightVector() *
        Offset;
}