// ============================================================
// RhythmInputComponent
//  役割 : キーボード(D/F/J/K)とスティック弾きを検出し、
//         4レーンの入力イベントに変換する
//  出口 : OnLaneInput(FLaneInput)
//  Lane : 0=L←, 1=L→, 2=R←, 3=R→
// Timestampは現在ワールド時間(仮)。後Cで楽曲時間に差し替える
// ============================================================
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RhythmInputTypes.h"
#include "RhythmInputComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLaneInput, const FLaneInput&, Input);

// スティック1本分の状態
struct FStickState
{
	bool bArmed = true;				// 発火できる状態か
	double LastFireTime = -100.0f;	// 最後に発火した時刻
	int32 LastDir = 0;				// 最後に発火した向き(-1=左 +1=右)
};

UCLASS(ClassGroup = (Rhythm), meta = (BlueprintSpawnableComponent))
class MUSICGAME_API URhythmInputComponent : public UActorComponent
{
	GENERATED_BODY()


public:
	URhythmInputComponent();

	UPROPERTY(BluePrintAssignable)
	FOnLaneInput OnLaneInput;

	void FireLane(int32 Lane, EInputType Type);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction)override;

private:
	void UpdateStick(float x, FStickState& State, int32 LeftLane, int32 RightLane, double Now);
	TMap<FKey, int32> KeyToLane;	// D,F,J,K →0..3
	TSet<FKey> HeldKeys;			// 押しっぱなしの連続発火を防ぐ

	FStickState LeftStick;
	FStickState RightStick;

	// 調整用の値(後でDataAssetに移す)
	float FireThreshold = 0.65f;	// これ以上倒れたら発火
	float ReleaseThreshold = 0.3f;	// これ以下に戻ったら再受付
	float ReverseLockSec = 0.06f;	// 発火後、逆方向を無効にする秒数

	// デバッグ表示
	bool bShowDebug = true;
	int32 LastLane = -1;
	double LastLaneTime = 0.0f;
	int32 FireCount = 0;
};