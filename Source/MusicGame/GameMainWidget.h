// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "GameMainWidget.generated.h"

class UMediaPlayer;
class UMediaSource;
class UTextBlock;
class UProgressBar;

// 判定の種類(判定を担当する人が、これを渡して ShowJudgement を呼ぶ)
UENUM(BlueprintType)
enum class EJudgement : uint8
{
	Perfect,
	Great,
	Good,
	Bad,
	Miss
};

/**
 * ゲーム本編の画面(スコア・体力・コンボ・判定の表示 + 背景動画)
 *  表示を変えたい人は、下の Set? / Show? に値を渡すだけでよい
 */
UCLASS()
class MUSICGAME_API UGameMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- 動画 ----
	UFUNCTION(BlueprintCallable, Category = "Video")
	void PlayVideos();

	UFUNCTION(BlueprintCallable, Category = "Video")
	void StopVideos();

	// ---- 表示の更新(スコア・体力・コンボ・判定を担当する人が呼ぶ) ----
	// 値を渡すだけで、画面の文字やゲージが変わる

	UFUNCTION(BlueprintCallable, Category = "GameMain")
	void SetScore(int32 Score);                  // 例: 1234 → 「01234」

	// 体力: 数字(HPText)とゲージ(HPBar)を更新する
	// 例: SetHP(800, 1000) → 「体力:800」、ゲージは 80%
	UFUNCTION(BlueprintCallable, Category = "GameMain")
	void SetHP(int32 CurrentHP, int32 MaxHP);

	UFUNCTION(BlueprintCallable, Category = "GameMain")
	void SetCombo(int32 Combo);                  // 0 以下のときは非表示

	UFUNCTION(BlueprintCallable, Category = "GameMain")
	void ShowJudgement(EJudgement Judgement);    // 判定を出して、少ししたら消す

protected:
	virtual void NativeConstruct() override;

	// ---- 動画 ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMediaPlayer> Player1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMediaSource> Source1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMediaPlayer> Player2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMediaSource> Source2;

	// 判定を表示しておく秒数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameMain")
	float JudgementDisplaySeconds = 0.5f;

	// ---- WBP側に、同じ名前の部品を置く(「変数である」にチェック) ----
	// BindWidgetOptional: 無くてもビルド・起動はできる(ただし、その表示は更新されない)
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HPText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> HPBar;   // 体力ゲージ(Progress Bar)
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ComboText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> JudgementText;

private:
	// 判定の文字を消す(タイマーから呼ばれる)
	UFUNCTION() void HideJudgement();

	FTimerHandle JudgementTimer;
};