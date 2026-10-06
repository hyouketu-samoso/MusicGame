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

// コンボが継続する判定か（グッド以上で継続、バット以下で途切れる）
inline bool KeepsCombo(ERhythmJudgement Judgement)
{
	return Judgement <= ERhythmJudgement::Good;
}
