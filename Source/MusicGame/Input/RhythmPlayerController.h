// ============================================================
// RhythmPlayerController
//  役割 : RhythmInputComponent を持ち、ポーズの開閉を管理する
//	ポーズ : Esc /Start -> OnPauseInput -> TogglePause
// ============================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RhythmPlayerController.generated.h"

class URhythmInputComponent;
class UUserWidget;
class ABackVideoActor;
class USongSelectWidget;
class UTimingAdjustWidget;
class USoundBase;
class ARhythmMusicPlayer;
class ANoteSpawner;

UCLASS()
class MUSICGAME_API ARhythmPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARhythmPlayerController();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<URhythmInputComponent>RhythmInput;

	// ポーズ画面のWidget(BPで WBP_PauseMenu を指定する)
	UPROPERTY(EditAnyWhere, BluePrintReadOnly, Category = "Rhythm|Pause")
	TSubclassOf<UUserWidget>PauseWidgetClass;

	// ポーズ画面を開いたときのSE(BPで指定する)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm|Pause|SE")
	TObjectPtr<USoundBase> PauseOpenSE;

	//　ポーズの開閉を切り替える
	UFUNCTION(BluePrintCallable, Category = "Rhythm|Pause")
	void TogglePause();

	// ポーズを解除する(再開ボタンにも使用可)
	UFUNCTION(BluePrintCallable, Category = "Rhythm|Pause")
	void ResumeGame();

	// ゲームを終了する(強制終了ボタン)
	UFUNCTION(BluePrintCallable, Category = "Rhythm|Pause")
	void QuitToWindows();

	// ゲーム画面のWidget(BPでWBP_GameMainを指定する)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget>MainWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	TSubclassOf<ABackVideoActor> BackVideoClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	FTransform BackVideoTransform;

	UPROPERTY()
	TObjectPtr<ABackVideoActor> BackVideo;

	// 調整画面を開いている間、ポーズの開閉入力を無視する（WBP から呼ぶ）
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Pause")
	void SetPauseInputBlocked(bool bBlocked);

protected:
	virtual void BeginPlay() override;

private:
	void OpenPause();

	UFUNCTION()
	void HandlePauseInput();

	// 曲変更通知を受け取る
	UFUNCTION()
	void HandleSongChangeRequested(int32 Direction);

	// 選択中の曲名を更新する
	void RefreshSongSelectInfo();

	UPROPERTY()
	TObjectPtr<UUserWidget>PauseWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget>MainWidget;

	bool bIsPaused = false;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<USongSelectWidget> SongSelectWidgetClass;

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category = "UI")
	TSubclassOf<UTimingAdjustWidget>TimingAdjustWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<USongSelectWidget> SongSelectWidget;

	UPROPERTY()
	TObjectPtr<ARhythmMusicPlayer> MusicPlayer;

	UFUNCTION()
	void HandleSongSelectClosed();

	UPROPERTY()
	TObjectPtr<UTimingAdjustWidget> TimingAdjustWidget;

	UFUNCTION()
	void HandleTimingAdjustRequested();

	UFUNCTION()
	void HandleTimingAdjustClosed();

	UFUNCTION()
	void HandleStartRequested();

	// 譜面を生成するNoteSpawner
	UPROPERTY()
	TObjectPtr<class ANoteSpawner> NoteSpawner;

	bool bStartRequested = false;

	bool bPauseInputBlocked = false;
	double PauseInputIgnoreUntil = 0.0;
};