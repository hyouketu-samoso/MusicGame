// 音ゲー共通の型定義

#pragma once

#include "CoreMinimal.h"
#include "RhythmTypes.generated.h"

// 判定の種類（上から良い順）
UENUM(BlueprintType)
enum class ERhythmJudgement : uint8
{
	Perfect,
	Great,
	Good,
	Bad,
	Miss,
};

// どのタイミングの判定か
UENUM(BlueprintType)
enum class ENoteJudgePoint : uint8
{
	Tap,        // 通常ノーツ
	HoldStart,  // ホールドの始点（押したとき）
	HoldEnd,    // ホールドの終点（離したとき）
};

// 判定ごとの値の表（判定倍率・体力変化など）
USTRUCT(BlueprintType)
struct FRhythmJudgementValues
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	float Perfect = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	float Great = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	float Good = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	float Bad = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	float Miss = 0.0f;

	float Get(ERhythmJudgement Judgement) const
	{
		switch (Judgement)
		{
		case ERhythmJudgement::Perfect: return Perfect;
		case ERhythmJudgement::Great:   return Great;
		case ERhythmJudgement::Good:    return Good;
		case ERhythmJudgement::Bad:     return Bad;
		default:                        return Miss;
		}
	}
};

// コンボ倍率の段階（MinCombo 以上でこの倍率）
USTRUCT(BlueprintType)
struct FComboMultiplierTier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm", meta=(ClampMin=0))
	int32 MinCombo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm", meta=(ClampMin=0.0))
	float Multiplier = 1.0f;
};

// コンボが継続する判定か（グッド以上で継続、バット以下で途切れる）
inline bool KeepsCombo(ERhythmJudgement Judgement)
{
	return Judgement <= ERhythmJudgement::Good;
}
