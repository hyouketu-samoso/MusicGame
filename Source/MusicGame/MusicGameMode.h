#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NoteManager.h"
#include "NoteData.h"
#include "MusicGameMode.generated.h"

UCLASS()
class AMusicGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere)
    USoundBase* MusicSound;

    UPROPERTY(EditAnywhere)
    float SpawnOffset = 1.5f;

protected:
    UPROPERTY()
    ANoteManager* NoteManager;

    UPROPERTY()
    UAudioComponent* MusicAudioComponent;

    TArray<FNoteData> Notes;

    float StartTime = 0.0f;   // ★ 曲の再生開始時間（Quartz の代わり）
};
