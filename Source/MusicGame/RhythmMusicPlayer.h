
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RhythmMusicPlayer.generated.h"

class UAudioComponent;
class USongData;

UCLASS()
class MUSICGAME_API ARhythmMusicPlayer : public AActor
{
    GENERATED_BODY()

public:
    ARhythmMusicPlayer();

    // 曲番号を指定して再生（0始まり）
    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlaySongByIndex(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlayMusic();

    UFUNCTION(BlueprintCallable, Category = "Music")
    void StopMusic();

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PauseMusic();

    UFUNCTION(BlueprintCallable, Category = "Music")
    void ResumeMusic();

    // 現在選択されている曲データ
    UFUNCTION(BlueprintPure, Category = "Music")
    USongData* GetCurrentSongData() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Music")
    TObjectPtr<UAudioComponent> AudioComponent;

    // SongDataアセットを登録する配列
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
    TArray<TObjectPtr<USongData>> Songs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Music")
    TObjectPtr<USongData> CurrentSongData;
};
