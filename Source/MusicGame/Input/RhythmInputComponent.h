// ============================================================
// RhythmInputComponent
//  役割 : キーボード(D/F/J/K)とスティック弾きを検出し、
//         4レーンの入力イベントに変換する
//  出口 : OnLaneInput(FLaneInput)
//		   OnPauseInput() ←Esc/Startボタン
//  Lane : 0=L←, 1=L→, 2=R←, 3=R→
// Timestampは現在ワールド時間(仮)。後Cで楽曲時間に差し替える
// ============================================================
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RhythmInputTypes.h"
#include "RhythmInputComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLaneInput, const FLaneInput&, Input);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseInput);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLaneRelease, const FLaneInput&, Input);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLaneHold, const FLaneInput&, Input);

// スティック1本分の状態
struct FStickState
{
	bool bArmed = true;				// 発火できる状態か
	double LastFireTime = -100.0f;	// 最後に発火した時刻
	int32 LastDir = 0;				// 最後に発火した向き(-1=左 +1=右)
	int32 HeldLane = -1;
};

UCLASS(ClassGroup = (Rhythm), meta = (BlueprintSpawnableComponent))
class MUSICGAME_API URhythmInputComponent : public UActorComponent
{
	GENERATED_BODY()


public:
	URhythmInputComponent();

	UPROPERTY(BlueprintAssignable)
	FOnLaneInput OnLaneInput;

	// Esc / Startボタンが押された瞬間に発火
	UPROPERTY(BlueprintAssignable)
	FOnPauseInput OnPauseInput;

	void FireLane(int32 Lane, EInputType Type);

	// レーンを話した瞬間に発火(TimeStampは離した時刻)
	UPROPERTY(BlueprintAssignable,Category = "Rhythm|Hold")
	FOnLaneRelease OnLaneRelease;

	// そのレーンが今押されている(押され続けている)か
	UFUNCTION(BlueprintPure,Category = "Rhythm|Hold")
	bool IsLaneHeld(int32 Lane)const;

	// 倒し続けて(押し続けて)この秒数たったら、ホールドとみなす
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Hold",
		meta = (ClampMin = "0.0"))
	float HoldStartSec = 0.15f;

	// ホールドが始まった瞬間に1回発火(フリックとは別の判定)
	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Hold")
	FOnLaneHold OnLaneHoldStart;

	// ホールドしていたレーンを離した瞬間に発火(HeldSecに押していた秒数)
	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Hold")
	FOnLaneHold OnLaneHoldEnd;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Stick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FireThreshold = 0.70f;	// 入力成立値(これ以上倒れたら発火)

	UPROPERTY(EditAnywhere, Category = "Rhythm|Stick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReleaseThreshold = 0.20f;	// デットゾーン(これ以下に戻ったら再受付)


public:
	// デッドゾーン(0.10〜0.35、0.05刻み)
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Stick")
	void SetDeadZone(float NewDeadZone);

	// 感度(1〜5)→入力成立値に変換
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Stick")
	void SetSensitivity(int32 Level);

	UFUNCTION(BlueprintPure, Category = "Rhythm|Stick")
	float GetDeadZone() const { return ReleaseThreshold; }

	UFUNCTION(BlueprintPure, Category = "Rhythm|Stick")
	int32 GetSensitivity() const { return SensitivityLevel; }

	// UIの◀▶ボタン用(Delta = -1 / +1)。変更後に自動保存
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Stick")
	void ChangeDeadZoneStep(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Stick")
	void ChangeSensitivityStep(int32 Delta);

	// 振動
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Vibration")
	void SetVibrationEnabled(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Vibration")
	void ToggleVibration();

	UFUNCTION(BlueprintPure, Category = "Rhythm|Vibration")
	bool IsVibrationEnabled() const { return bVibrationEnabled; }

	void SaveInputSettings();
	void LoadInputSettings();

private:
	int32 SensitivityLevel = 3;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction)override;
	virtual void BeginPlay() override;

private:
	void UpdateStick(float x, FStickState& State, int32 LeftLane, int32 RightLane, double Now);
	void UpdatePause(APlayerController* PC);
	void ReleaseLane(int32 Lane);
	void ReleaseAllLanes();
	void UpdateHold(double Now);

	TMap<FKey, int32> KeyToLane;	// D,F,J,K →0..3
	TSet<FKey> HeldKeys;			// 押しっぱなしの連続発火を防ぐ

	FStickState LeftStick;
	FStickState RightStick;

	bool bPauseHeld = false;		// ポーズキーの押しっぱなし防止

	bool bVibrationEnabled = true;

	// 調整用の値(後でDataAssetに移す)
	float ReverseLockSec = 0.06f;	// 発火後、逆方向を無効にする秒数

	// デバッグ表示
	bool bShowDebug = true;
	int32 LastLane = -1;
	double LastLaneTime = 0.0f;
	int32 FireCount = 0;

	bool bLaneHeld[4] = { false,false,false,false };
	EInputType LaneHeldType[4] = { EInputType::Key, EInputType::Key, EInputType::Key, EInputType::Key };

	double LanePressTime[4] = { 0, 0, 0, 0 };
	bool bHoldStarted[4] = { false, false, false, false };
};