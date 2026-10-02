// ============================================================
// RhythmInputTypes
//  役割 : 入力まわりで共有する型の定義(他の担当者との境界)
//  内容 : EInputType  = 入力の種類(Flick / Key)
//         FLaneInput  = 1回の入力イベント(Lane, Timestamp, InputType)
//  Lane : 0=L←, 1=L→, 2=R←, 3=R→
//  Timestampは現在 仮。後で楽曲時間に差し替える
// ============================================================
#pragma once
#include "CoreMinimal.h"
#include "RhythmInputTypes.generated.h"

UENUM(BlueprintType)
enum class EInputType : uint8{Flick,Key};

USTRUCT(BlueprintType)
struct FLaneInput
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)int32 Lane = 0;		// 0=L←, 1=L→, 2=R←, 3=R→
	UPROPERTY(BlueprintReadWrite) double Timestamp = 0;	// 後で楽曲時間にする
	UPROPERTY(BlueprintReadWrite)EInputType InputType = EInputType::Key;
};