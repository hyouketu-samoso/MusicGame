// スコア・コンボ・体力を管理するコンポーネント（仕様書「音ゲースコアと体力仕様」準拠）。
// 判定役（ARhythmJudge）が判定を出すたびに AddJudgement を呼ぶ。
//
// スコア：1判定の内部スコア = 判定倍率 × コンボ倍率
//         最終スコア = 獲得内部スコア合計 ÷ 理論最大内部スコア × 100（0.00〜100.00点）
// コンボ：Perfect / Great / Good で +1、Bad / Miss で 0 に戻る
// 体力  ：開始 100、Bad −8、Miss −15、回復なし。0 以下でゲームオーバー

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RhythmTypes.h"
#include "RhythmScoreComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSongFinished, bool, bCleared, double, FinalScore);

UCLASS(ClassGroup=(Rhythm), meta=(BlueprintSpawnableComponent))
class URhythmScoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	URhythmScoreComponent();

	// 判定を1つ反映する（コンボ → スコア → 体力 の順に更新）
	void AddJudgement(ERhythmJudgement Judgement);

	// 譜面の理論最大を決めるための判定数を設定する（ホールドは始点・終点で 2 と数える）
	// 0 のままなら「ここまでに判定した数」を理論最大の基準にする（終わりのないテスト用）
	UFUNCTION(BlueprintCallable, Category="Rhythm Score")
	void SetTotalJudgeCount(int32 Count) { TotalJudgeCount = FMath::Max(Count, 0); }

	// 楽曲終了時に呼ぶ。クリア判定（体力 1 以上）をして OnSongFinished を発行する
	UFUNCTION(BlueprintCallable, Category="Rhythm Score")
	bool FinishSong();

	// スコア・コンボ・体力を開始時の状態に戻す
	UFUNCTION(BlueprintCallable, Category="Rhythm Score")
	void ResetResult();

	// 現在のスコア（0〜100、丸めなし）
	UFUNCTION(BlueprintPure, Category="Rhythm Score")
	double GetScore() const;

	// 表示用スコア（小数第2位に丸めたもの）
	UFUNCTION(BlueprintPure, Category="Rhythm Score")
	double GetDisplayScore() const;

	// 指定コンボ数のときのコンボ倍率
	UFUNCTION(BlueprintPure, Category="Rhythm Score")
	float GetComboMultiplier(int32 ComboCount) const;

	UFUNCTION(BlueprintPure, Category="Rhythm Score")
	int32 GetCombo() const { return Combo; }

	UFUNCTION(BlueprintPure, Category="Rhythm Score")
	int32 GetMaxCombo() const { return MaxCombo; }

	UFUNCTION(BlueprintPure, Category="Rhythm Score")
	int32 GetJudgementCount(ERhythmJudgement Judgement) const;

	UFUNCTION(BlueprintPure, Category="Rhythm Score")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category="Rhythm Score")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Rhythm Score")
	bool IsGameOver() const { return bGameOver; }

	// 体力が変わったとき
	UPROPERTY(BlueprintAssignable, Category="Rhythm Score")
	FOnHealthChanged OnHealthChanged;

	// 体力が 0 以下になったとき（その時点で終了）
	UPROPERTY(BlueprintAssignable, Category="Rhythm Score")
	FOnGameOver OnGameOver;

	// FinishSong が呼ばれたとき
	UPROPERTY(BlueprintAssignable, Category="Rhythm Score")
	FOnSongFinished OnSongFinished;

protected:

	virtual void BeginPlay() override;

	// 判定倍率（Perfect 1.0 / Great 0.8 / Good 0.5 / Bad 0.2 / Miss 0）
	UPROPERTY(EditAnywhere, Category="Rhythm Score|Score")
	FRhythmJudgementValues JudgementRates;

	// コンボ倍率の段階（MinCombo の小さい順に並べる）
	UPROPERTY(EditAnywhere, Category="Rhythm Score|Score")
	TArray<FComboMultiplierTier> ComboMultiplierTiers;

	// 初期体力
	UPROPERTY(EditAnywhere, Category="Rhythm Score|Health", meta=(ClampMin=1.0))
	float InitialHealth = 100.0f;

	// 最大体力
	UPROPERTY(EditAnywhere, Category="Rhythm Score|Health", meta=(ClampMin=1.0))
	float MaxHealth = 100.0f;

	// 判定ごとの体力変化（Bad −8 / Miss −15、それ以外 0）
	UPROPERTY(EditAnywhere, Category="Rhythm Score|Health")
	FRhythmJudgementValues HealthChanges;

	// true なら体力が 0 になってもゲームオーバーにしない（テスト用）
	UPROPERTY(EditAnywhere, Category="Rhythm Score|Debug")
	bool bInvincible = false;

private:

	// 判定数 Count 回をすべて Perfect・コンボ継続で取ったときの内部スコア合計
	double CalcMaxInternalScore(int32 Count) const;

	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Score|Result")
	int32 Combo = 0;

	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Score|Result")
	int32 MaxCombo = 0;

	// 判定ごとの回数（ERhythmJudgement の順）
	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Score|Result")
	TArray<int32> JudgementCounts;

	// 獲得した内部スコアの合計（途中では丸めない）
	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Score|Result")
	double EarnedInternalScore = 0.0;

	// ここまでに判定した数
	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Score|Result")
	int32 JudgedCount = 0;

	// 譜面全体の判定数（0 = 未設定）
	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Score|Result")
	int32 TotalJudgeCount = 0;

	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Score|Result")
	float Health = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Score|Result")
	bool bGameOver = false;
};
