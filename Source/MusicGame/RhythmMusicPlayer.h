
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

    // 曲を選択して再生する
    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlaySongByIndex(int32 Index);

    // 現在選択されている曲を再生する
    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlayMusic();

    UFUNCTION(BlueprintCallable, Category = "Music")
    void StopMusic();

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PauseMusic();

    UFUNCTION(BlueprintCallable, Category = "Music")
    void ResumeMusic();

    // 現在選択中の曲データを取得
    UFUNCTION(BlueprintPure, Category = "Music")
    USongData* GetCurrentSongData() const;

    // 曲選択画面からの通知
    UFUNCTION()
    void HandleSongChangeRequested(int32 Direction);

    // 曲数を取得
    UFUNCTION(BlueprintPure, Category = "Music")
    int32 GetSongCount() const;

    // 指定した番号の曲データを取得
    UFUNCTION(BlueprintPure, Category = "Music")
    USongData* GetSongDataByIndex(int32 Index) const;

    // 現在の曲番号を取得
    UFUNCTION(BlueprintPure, Category = "Music")
    int32 GetCurrentSongIndex() const;

protected:
    virtual void BeginPlay() override;

    // 音声再生コンポーネント
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Music")
    TObjectPtr<UAudioComponent> AudioComponent;

    // 再生可能な曲データ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
    TArray<TObjectPtr<USongData>> Songs;

    // 現在選択中の曲
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Music")
    TObjectPtr<USongData> CurrentSongData;

    // 現在選択中の曲番号
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Music")
    int32 CurrentSongIndex = INDEX_NONE;
};
