// Fill out your copyright notice in the Description page of Project Settings.
// ============================================================
// SongSelectWidget
//  役割 : ゲーム開始前の曲選択UIの骨組み
//         (曲の切替 / 難易度 / 速度 / タイミング調整 / ゲームスタート)
//  方針 : ここでは「押された」ことを通知するだけで、
//         速度などの実際の処理は、担当者がデリゲートに繋いで行う
//  演出 : ゲームスタートで、フェードアウト(+少し拡大)してから自分を消す
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SongSelectWidget.generated.h"

class UButton;
class UTextBlock;

UENUM(BlueprintType)
enum class ESongDifficulty : uint8
{
	Normal,
	Hard
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSongChangeRequested, int32, Direction);       // -1=前の曲 / +1=次の曲
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDifficultySelected, ESongDifficulty, Difficulty);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeedSelected, int32, Speed);                  // 1?5
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimingAdjustRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStartRequested);   // ゲームスタートが押された瞬間
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSongSelectClosed);     // フェードが終わって、ウィジェットが消えた後

UCLASS()
class MUSICGAME_API USongSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- 通知(処理の担当者はここに繋ぐ) ----
	UPROPERTY(BlueprintAssignable, Category = "SongSelect")
	FOnSongChangeRequested OnSongChangeRequested;

	UPROPERTY(BlueprintAssignable, Category = "SongSelect")
	FOnDifficultySelected OnDifficultySelected;

	UPROPERTY(BlueprintAssignable, Category = "SongSelect")
	FOnSpeedSelected OnSpeedSelected;

	UPROPERTY(BlueprintAssignable, Category = "SongSelect")
	FOnTimingAdjustRequested OnTimingAdjustRequested;

	UPROPERTY(BlueprintAssignable, Category = "SongSelect")
	FOnGameStartRequested OnStartRequested;

	UPROPERTY(BlueprintAssignable, Category = "SongSelect")
	FOnSongSelectClosed OnClosed;

	// ---- 表示の更新(処理の担当者から呼ぶ) ----
	// 曲名と「ノーツ数 / 分:秒」を表示する
	UFUNCTION(BlueprintCallable, Category = "SongSelect")
	void SetSongInfo(const FText& Title, int32 NoteCount, float LengthSeconds);

	// 選択中の見た目だけを変える(通知は飛ばさない)
	UFUNCTION(BlueprintCallable, Category = "SongSelect")
	void SetSelectedDifficulty(ESongDifficulty Difficulty);

	UFUNCTION(BlueprintCallable, Category = "SongSelect")
	void SetSelectedSpeed(int32 Speed);

	// フェードアウトして消す(ゲームスタートからも呼ばれる)
	UFUNCTION(BlueprintCallable, Category = "SongSelect")
	void CloseWithFade();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ---- 演出の調整値 ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SongSelect|Fade")
	float FadeDuration = 0.5f;          // 消えるまでの秒数

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SongSelect|Fade")
	float ScaleUpAmount = 0.06f;        // 消えるときに、どれだけ拡大するか(0.06 = 6%)

	// ---- 選択中の色 ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SongSelect|Color")
	FLinearColor SelectedColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SongSelect|Color")
	FLinearColor NormalColor = FLinearColor::White;

	// ---- WBP側に、同じ名前の部品を置く(「変数である」にチェック) ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SongTitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SongInfoText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SongPrevButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SongNextButton;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> NormalButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> HardButton;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SpeedButton1;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SpeedButton2;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SpeedButton3;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SpeedButton4;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SpeedButton5;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> TimingButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> StartButton;

private:
	// ボタンのクリック(AddDynamic には UFUNCTION が必要)
	UFUNCTION() void HandleSongPrev();
	UFUNCTION() void HandleSongNext();
	UFUNCTION() void HandleNormal();
	UFUNCTION() void HandleHard();
	UFUNCTION() void HandleSpeed1();
	UFUNCTION() void HandleSpeed2();
	UFUNCTION() void HandleSpeed3();
	UFUNCTION() void HandleSpeed4();
	UFUNCTION() void HandleSpeed5();
	UFUNCTION() void HandleTiming();
	UFUNCTION() void HandleStart();

	void SelectSpeed(int32 Speed);

	UPROPERTY()
	TArray<TObjectPtr<UButton>> SpeedButtons;

	bool bClosing = false;
	float ElapsedClose = 0.0f;
};