// ============================================================
// RhythmInputComponent
//  役割 : キーボード(D/F/J/K)とスティック弾きを検出し、
//         4レーンの入力イベントに変換する
//  出口 : OnLaneInput(FLaneInput)
//		   OnPauseInput() ←Esc/Startボタン
//  Lane : 0=L←, 1=L→, 2=R←, 3=R→
// Timestampは現在ワールド時間(仮)。後で楽曲時間に差し替える
// ============================================================
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RhythmInputTypes.h"
#include "RhythmInputComponent.generated.h"

class USoundMix;
class USoundClass;
class UMaterialParameterCollection;

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
	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Hold")
	FOnLaneRelease OnLaneRelease;

	// そのレーンが今押されている(押され続けている)か
	UFUNCTION(BlueprintPure, Category = "Rhythm|Hold")
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

	// 音量(0～10) UIの◀▶

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

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void ChangeBGMVolumeStep(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void ChangeSEVolumeStep(int32 Delta);

	UFUNCTION(BlueprintPure, Category = "Rhythm|Audio")
	int32 GetBGMVolumeLevel() const { return BGMVolumeLevel; }

	UFUNCTION(BlueprintPure, Category = "Rhythm|Audio")
	int32 GetSEVolumeLevel() const { return SEVolumeLevel; }

	// 音量オーバーライドを載せる Sound Mix(中身は空でよい)
	UPROPERTY(EditAnywhere, Category = "Rhythm|Audio")
	TObjectPtr<USoundMix> SoundMix;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Audio")
	TObjectPtr<USoundClass> BGMClass;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Audio")
	TObjectPtr<USoundClass> SEClass;

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void ChangeMasterVolumeStep(int32 Delta);

	UFUNCTION(BlueprintPure, Category = "Rhythm|Audio")
	int32 GetMasterVolumeLevel() const { return MasterVolumeLevel; }

	// スライダー用 値を直接設定して反映する
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void SetMasterVolumeLevel(int32 Level);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void SetBGMVolumeLevel(int32 Level);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void SetSEVolumeLevel(int32 Level);

	// 音量の保存。スライダーを操作し終えたときに呼ぶ
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Audio")
	void SaveVolumeSettings();

	// ---- 明るさ(背景 / レーン / ノーツ) ----
	// 段階は 0〜10。5が標準でスライダーのちょうど真ん中(背景=元の明るさ / ノーツ=本来の色)
	// 段階と倍率の対応は、下の ○○ScaleAtMin / AtMid / AtMax で決まる
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Brightness")
	void SetBGBrightnessLevel(int32 Level);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Brightness")
	void SetLaneBrightnessLevel(int32 Level);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Brightness")
	void SetNoteBrightnessLevel(int32 Level);

	// ◀▶ボタン用(Delta = -1 / +1。1回で1段階動く)。変更後に自動保存
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Brightness")
	void ChangeBGBrightnessStep(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Brightness")
	void ChangeLaneBrightnessStep(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Brightness")
	void ChangeNoteBrightnessStep(int32 Delta);

	UFUNCTION(BlueprintPure, Category = "Rhythm|Brightness")
	int32 GetBGBrightnessLevel() const { return BGBrightnessLevel; }

	UFUNCTION(BlueprintPure, Category = "Rhythm|Brightness")
	int32 GetLaneBrightnessLevel() const { return LaneBrightnessLevel; }

	UFUNCTION(BlueprintPure, Category = "Rhythm|Brightness")
	int32 GetNoteBrightnessLevel() const { return NoteBrightnessLevel; }

	// スライダーを操作し終えたときに呼ぶ
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Brightness")
	void SaveBrightnessSettings();

	// MPC_Brightness を指定する(BP_RhythmPlayerController のこのコンポーネントで設定)
	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness")
	TObjectPtr<UMaterialParameterCollection> BrightnessCollection;

	// 段階の範囲(Designer のスライダーも Min=0 / Max=10 / Step Size=1 にする)
	static constexpr int32 BrightnessMinStep = 0;
	static constexpr int32 BrightnessMidStep = 5;	// 標準
	static constexpr int32 BrightnessMaxStep = 10;

	// 背景: 0段階目 / 5段階目 / 10段階目 の倍率
	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float BGScaleAtMin = 0.02f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float BGScaleAtMid = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float BGScaleAtMax = 1.5f;

	// ノーツ: MPC の Note に渡す値(マテリアル側で Note の4乗を掛けている想定)
	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float NoteScaleAtMin = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float NoteScaleAtMid = 1.0f;	// 今の10の色が、標準の5で出る

	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float NoteScaleAtMax = 1.3f;	// 10は、標準より少し明るい側

	// レーン: 素材が決まったら調整する(今は仮の値)
	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float LaneScaleAtMin = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float LaneScaleAtMid = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Rhythm|Brightness", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float LaneScaleAtMax = 1.0f;


	void SaveInputSettings();
	void LoadInputSettings();

private:
	int32 SensitivityLevel = 3;
	int32 BGMVolumeLevel = 50;
	int32 SEVolumeLevel = 50;
	int32 MasterVolumeLevel = 50;

	// 明るさの段階(0〜10。5が標準)
	int32 BGBrightnessLevel = 5;
	int32 LaneBrightnessLevel = 5;
	int32 NoteBrightnessLevel = 5;

	bool bVolumeDirty = false;
	bool bBrightnessDirty = false;

	void ApplyVolumes();
	void ApplyBrightness();

	// 段階(0〜10)を倍率に変える。0段階目=AtMin / 5段階目=AtMid / 10段階目=AtMax の間を直線でつなぐ
	float StepToScale(int32 Step, float AtMin, float AtMid, float AtMax) const;


protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction)override;
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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