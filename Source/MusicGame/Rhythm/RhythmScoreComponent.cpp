#include "RhythmScoreComponent.h"

URhythmScoreComponent::URhythmScoreComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 判定倍率
	JudgementRates.Perfect = 1.0f;
	JudgementRates.Great   = 0.8f;
	JudgementRates.Good    = 0.5f;
	JudgementRates.Bad     = 0.2f;
	JudgementRates.Miss    = 0.0f;

	// コンボ倍率（0〜9: 1.00 / 10〜29: 1.05 / 30〜49: 1.10 / 50〜99: 1.15 / 100以上: 1.20）
	ComboMultiplierTiers = {
		{ 0,   1.00f },
		{ 10,  1.05f },
		{ 30,  1.10f },
		{ 50,  1.15f },
		{ 100, 1.20f },
	};

	// 体力変化（回復なし）
	HealthChanges.Bad  = -8.0f;
	HealthChanges.Miss = -15.0f;

	JudgementCounts.Init(0, StaticEnum<ERhythmJudgement>()->NumEnums() - 1);
}

void URhythmScoreComponent::BeginPlay()
{
	Super::BeginPlay();

	// 詳細パネルで変えた初期体力を反映する
	ResetResult();
}

void URhythmScoreComponent::AddJudgement(ERhythmJudgement Judgement)
{
	// ゲームオーバー後は何も反映しない
	if (bGameOver)
	{
		return;
	}

	// コンボ：成功で +1、Bad / Miss で 0 に戻る
	if (KeepsCombo(Judgement))
	{
		++Combo;
		MaxCombo = FMath::Max(MaxCombo, Combo);
	}
	else
	{
		Combo = 0;
	}

	++JudgementCounts[static_cast<int32>(Judgement)];

	// スコア：判定倍率 × コンボ倍率。
	// コンボ倍率は「この判定を反映した後のコンボ数」で決める（Bad / Miss はリセット後の ×1.00）
	EarnedInternalScore += JudgementRates.Get(Judgement) * GetComboMultiplier(Combo);
	++JudgedCount;

	// 体力
	const float Delta = HealthChanges.Get(Judgement);
	if (Delta != 0.0f)
	{
		Health = FMath::Clamp(Health + Delta, 0.0f, MaxHealth);
		OnHealthChanged.Broadcast(Health, Delta);

		if (Health <= 0.0f && !bInvincible)
		{
			bGameOver = true;
			OnGameOver.Broadcast();
		}
	}
}

float URhythmScoreComponent::GetComboMultiplier(int32 ComboCount) const
{
	// 条件を満たす段階のうち、一番上のもの
	float Multiplier = 1.0f;
	for (const FComboMultiplierTier& Tier : ComboMultiplierTiers)
	{
		if (ComboCount >= Tier.MinCombo)
		{
			Multiplier = Tier.Multiplier;
		}
	}
	return Multiplier;
}

double URhythmScoreComponent::CalcMaxInternalScore(int32 Count) const
{
	// 全部 Perfect でコンボが続けば、k 回目の判定はコンボ数 k
	double Total = 0.0;
	for (int32 k = 1; k <= Count; ++k)
	{
		Total += JudgementRates.Perfect * GetComboMultiplier(k);
	}
	return Total;
}

double URhythmScoreComponent::GetScore() const
{
	// 譜面の判定数が分かっていればそれを、分からなければここまでの判定数を理論最大の基準にする
	const double MaxScore = CalcMaxInternalScore(TotalJudgeCount > 0 ? TotalJudgeCount : JudgedCount);
	if (MaxScore <= 0.0)
	{
		return 0.0;
	}
	return FMath::Clamp(EarnedInternalScore / MaxScore * 100.0, 0.0, 100.0);
}

double URhythmScoreComponent::GetDisplayScore() const
{
	// 表示するときだけ小数第2位に丸める
	return FMath::RoundHalfFromZero(GetScore() * 100.0) / 100.0;
}

bool URhythmScoreComponent::FinishSong()
{
	const bool bCleared = !bGameOver && Health >= 1.0f;
	OnSongFinished.Broadcast(bCleared, GetDisplayScore());
	return bCleared;
}

void URhythmScoreComponent::ResetResult()
{
	Combo = 0;
	MaxCombo = 0;
	for (int32& Count : JudgementCounts)
	{
		Count = 0;
	}
	EarnedInternalScore = 0.0;
	JudgedCount = 0;
	Health = FMath::Min(InitialHealth, MaxHealth);
	bGameOver = false;
}

int32 URhythmScoreComponent::GetJudgementCount(ERhythmJudgement Judgement) const
{
	const int32 Index = static_cast<int32>(Judgement);
	return JudgementCounts.IsValidIndex(Index) ? JudgementCounts[Index] : 0;
}
