
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

    // 曲を選択する（再生はしない）
    UFUNCTION(BlueprintCallable, Category = "Music")
    void SelectSongByIndex(int32 Index);

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


    // 音楽の再生開始を遅らせる時間（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Timing")
    float MusicStartDelay = 0.2f;

    FTimerHandle MusicStartTimerHandle;
protected:
    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
