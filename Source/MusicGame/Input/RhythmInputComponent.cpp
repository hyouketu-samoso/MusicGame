#include "RhythmInputComponent.h"
#include "Kismet/GamePlayStatics.h"
#include "RhythmSettingsSaveGame.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "InputCoreTypes.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"

URhythmInputComponent::URhythmInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;	//ポーズ中もEscを検出する

	KeyToLane.Add(EKeys::D, 0);
	KeyToLane.Add(EKeys::F, 1);
	KeyToLane.Add(EKeys::J, 2);
	KeyToLane.Add(EKeys::K, 3);
}

void URhythmInputComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return;

	// ポーズ入力(ポーズ中でも受け付ける)
	UpdatePause(PC);

	// ポーズ中はレーン入力を出さない(押されていたレーンは離した扱いにする)
	if (GetWorld()->IsPaused())
	{
		return;
	}

	// キーボード
	for (const TPair<FKey, int32>& Pair : KeyToLane)
	{
		const bool bDown = PC->IsInputKeyDown(Pair.Key);
		if (bDown && !HeldKeys.Contains(Pair.Key))
		{
			HeldKeys.Add(Pair.Key);
			FireLane(Pair.Value, EInputType::Key);
		}
		else if (!bDown)
		{
			// 押されていたキーが離されたら、離した通知を出す
			if (HeldKeys.Remove(Pair.Key) > 0)
			{
				ReleaseLane(Pair.Value);
			}
		}
	}

	// スティック
	const double Now = GetWorld()->GetTimeSeconds();
	const float LX = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float RX = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX);

	UpdateStick(LX, LeftStick, 0, 1, Now);	// L ← =0, L → =1
	UpdateStick(RX, RightStick, 2, 3, Now);	// R ← =2, R → =3
	UpdateHold(Now);

	if (bShowDebug && GEngine)
	{
		// キー(Key)を固定にすると、同じ行が毎フレーム上書きされる
		GEngine->AddOnScreenDebugMessage(100, 0.f, FColor::Cyan,
			FString::Printf(TEXT("Lane=%d t=%.3f count=%d hold=%d%d%d%d"),
				LastLane, LastLaneTime, FireCount,
				bLaneHeld[0], bLaneHeld[1], bLaneHeld[2], bLaneHeld[3]));

		GEngine->AddOnScreenDebugMessage(101, 0.f, FColor::Yellow,
			FString::Printf(TEXT("LX=%.3f RX=%.3f"), LX, RX));
	}
}

void URhythmInputComponent::UpdatePause(APlayerController* PC)
{
	// Esc または Startボタン(UE上では Gamepad_Special_Right)
	const bool bDown = PC->IsInputKeyDown(EKeys::Escape)
		|| PC->IsInputKeyDown(EKeys::Gamepad_Special_Right);

	if (bDown && !bPauseHeld)
	{
		bPauseHeld = true;
		OnPauseInput.Broadcast();
	}
	else if (!bDown)
	{
		bPauseHeld = false;
	}
}

void URhythmInputComponent::UpdateStick(float X, FStickState& State,
	int32 LeftLane, int32 RightLane, double Now)
{
	const float AbsX = FMath::Abs(X);
	const int32 Dir = (X < 0.f) ? -1 : 1;

	// 倒し続けている間は「押されている」。戻した・逆に倒したら離す
	if (State.HeldLane >= 0)
	{
		const int32 HeldDir = (State.HeldLane == LeftLane) ? -1 : 1;
		if (AbsX < ReleaseThreshold || Dir != HeldDir)
		{
			ReleaseLane(State.HeldLane);
			State.HeldLane = -1;
		}
	}

	// ニュートラルに戻ったら再受付
	if (AbsX < ReleaseThreshold)
	{
		State.bArmed = true;
		return;
	}

	if (!State.bArmed || AbsX < FireThreshold) return;

	// 発火直後の逆方向(戻りの勢い)は無視
	if (State.LastDir != 0 && Dir != State.LastDir
		&& (Now - State.LastFireTime) < ReverseLockSec)
	{
		return;
	}

	State.bArmed = false;
	State.LastDir = Dir;
	State.LastFireTime = Now;

	const int32 Lane = (Dir < 0) ? LeftLane : RightLane;
	State.HeldLane = Lane;
	FireLane(Lane, EInputType::Flick);
}

void URhythmInputComponent::FireLane(int32 Lane, EInputType Type)
{
	FLaneInput In;
	In.Lane = Lane;
	In.Timestamp = GetWorld()->GetTimeSeconds();	//後で楽曲時間にする
	In.InputType = Type;

	UE_LOG(LogTemp, Log, TEXT("Lane %d t = %.3f type=%s"), Lane, In.Timestamp,
		Type == EInputType::Flick ? TEXT("Flick") : TEXT("Key"));

	LastLane = Lane;
	LastLaneTime = In.Timestamp;
	++FireCount;

	// 押されている状態にする
	if (Lane >= 0 && Lane < 4)
	{
		bLaneHeld[Lane] = true;
		LanePressTime[Lane] = In.Timestamp;
		bHoldStarted[Lane] = false;
		LaneHeldType[Lane] = Type;
	}

	OnLaneInput.Broadcast(In);
}

bool URhythmInputComponent::IsLaneHeld(int32 Lane) const
{
	return Lane >= 0 && Lane < 4 && bLaneHeld[Lane];
}

void URhythmInputComponent::ReleaseLane(int32 Lane)
{
	if (Lane < 0 || Lane >= 4 || !bLaneHeld[Lane]) return;

	bLaneHeld[Lane] = false;

	FLaneInput In;
	In.Lane = Lane;
	In.Timestamp = GetWorld()->GetTimeSeconds();	//後で楽曲時間にする
	In.InputType = LaneHeldType[Lane];
	In.HeldSec = In.Timestamp - LanePressTime[Lane];

	UE_LOG(LogTemp, Log, TEXT("Release Lane %d t = %.3f held=%.3f"),
		Lane, In.Timestamp, In.HeldSec);

	OnLaneRelease.Broadcast(In);

	// ホールドが始まっていたなら、ホールド終了も通知する
	if (bHoldStarted[Lane])
	{
		bHoldStarted[Lane] = false;
		UE_LOG(LogTemp, Log, TEXT("HoldEnd Lane %d"), Lane);
		OnLaneHoldEnd.Broadcast(In);
	}
}

void URhythmInputComponent::ReleaseAllLanes()
{
	for (int32 Lane = 0; Lane < 4; ++Lane)
	{
		ReleaseLane(Lane);
	}
	LeftStick.HeldLane = -1;
	RightStick.HeldLane = -1;
}

void URhythmInputComponent::UpdateHold(double Now)
{
	for (int32 Lane = 0; Lane < 4; ++Lane)
	{
		if (!bLaneHeld[Lane] || bHoldStarted[Lane]) continue;

		const double Held = Now - LanePressTime[Lane];
		if (Held < HoldStartSec) continue;

		bHoldStarted[Lane] = true;

		FLaneInput In;
		In.Lane = Lane;
		In.Timestamp = Now;		//後で楽曲時間にする
		In.InputType = LaneHeldType[Lane];
		In.HeldSec = Held;

		UE_LOG(LogTemp, Log, TEXT("HoldStart Lane %d t = %.3f"), Lane, Now);

		OnLaneHoldStart.Broadcast(In);
	}
}

void URhythmInputComponent::SetDeadZone(float NewDeadZone)
{
	// 0.05刻みに丸めて、仕様書の範囲(0.10〜0.35)に収める
	const float Snapped = FMath::RoundToFloat(NewDeadZone / 0.05f) * 0.05f;
	float DZ = FMath::Clamp(Snapped, 0.10f, 0.35f);

	// 整合条件:デッドゾーン < 入力成立値
	DZ = FMath::Min(DZ, FireThreshold - 0.05f);

	ReleaseThreshold = DZ;
}

void URhythmInputComponent::SetSensitivity(int32 Level)
{
	// 感度1〜5 → 入力成立値(高感度ほど小さい)
	static const float FireByLevel[5] = { 0.80f, 0.75f, 0.70f, 0.65f, 0.60f };

	SensitivityLevel = FMath::Clamp(Level, 1, 5);
	FireThreshold = FireByLevel[SensitivityLevel - 1];

	// 成立値が変わったので、デッドゾーンも整合条件を再確認
	ReleaseThreshold = FMath::Min(ReleaseThreshold, FireThreshold - 0.05f);
}

static const TCHAR* GInputSettingsSlot = TEXT("RhythmInputSettings");

void URhythmInputComponent::BeginPlay()
{
	Super::BeginPlay();
	LoadInputSettings();	// 保存済みの設定を反映
	ApplyVolumes();			// 保存済みの音量を反映
	ApplyBrightness();		// 保存済みの明るさを反映
}

void URhythmInputComponent::ChangeDeadZoneStep(int32 Delta)
{
	SetDeadZone(ReleaseThreshold + 0.05f * Delta);
	SaveInputSettings();
}

void URhythmInputComponent::ChangeSensitivityStep(int32 Delta)
{
	SetSensitivity(SensitivityLevel + Delta);
	SaveInputSettings();
}

void URhythmInputComponent::SetVibrationEnabled(bool bEnable)
{
	bVibrationEnabled = bEnable;
	SaveInputSettings();
}

void URhythmInputComponent::ToggleVibration()
{
	SetVibrationEnabled(!bVibrationEnabled);
}

void URhythmInputComponent::SaveInputSettings()
{
	URhythmSettingsSaveGame* Save = Cast<URhythmSettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(URhythmSettingsSaveGame::StaticClass()));
	if (!Save) return;

	Save->DeadZone = ReleaseThreshold;
	Save->SensitivityLevel = SensitivityLevel;
	Save->bVibration = bVibrationEnabled;
	Save->BGMVolumeLevel = BGMVolumeLevel;
	Save->SEVolumeLevel = SEVolumeLevel;
	Save->MasterVolumeLevel = MasterVolumeLevel;
	Save->BGBrightnessStep = BGBrightnessLevel;
	Save->LaneBrightnessStep = LaneBrightnessLevel;
	Save->NoteBrightnessStep = NoteBrightnessLevel;

	UGameplayStatics::SaveGameToSlot(Save, GInputSettingsSlot, 0);

	UE_LOG(LogTemp, Warning, TEXT("Save: M=%d B=%d S=%d | Bright BG=%d Lane=%d Note=%d"),
		MasterVolumeLevel, BGMVolumeLevel, SEVolumeLevel,
		BGBrightnessLevel, LaneBrightnessLevel, NoteBrightnessLevel);
}

void URhythmInputComponent::LoadInputSettings()
{
	if (!UGameplayStatics::DoesSaveGameExist(GInputSettingsSlot, 0)) return;

	URhythmSettingsSaveGame* Load = Cast<URhythmSettingsSaveGame>(
		UGameplayStatics::LoadGameFromSlot(GInputSettingsSlot, 0));
	if (!Load) return;

	// 感度を先に入れる(デッドゾーンは入力成立値より小さくする必要があるため)
	SetSensitivity(Load->SensitivityLevel);
	SetDeadZone(Load->DeadZone);
	bVibrationEnabled = Load->bVibration;
	BGMVolumeLevel = FMath::Clamp(Load->BGMVolumeLevel, 0, 100);
	SEVolumeLevel = FMath::Clamp(Load->SEVolumeLevel, 0, 100);
	MasterVolumeLevel = FMath::Clamp(Load->MasterVolumeLevel, 0, 100);
	BGBrightnessLevel = FMath::Clamp(Load->BGBrightnessStep, BrightnessMinStep, BrightnessMaxStep);
	LaneBrightnessLevel = FMath::Clamp(Load->LaneBrightnessStep, BrightnessMinStep, BrightnessMaxStep);
	NoteBrightnessLevel = FMath::Clamp(Load->NoteBrightnessStep, BrightnessMinStep, BrightnessMaxStep);

	UE_LOG(LogTemp, Warning, TEXT("Load: M=%d B=%d S=%d | Bright BG=%d Lane=%d Note=%d"),
		MasterVolumeLevel, BGMVolumeLevel, SEVolumeLevel,
		BGBrightnessLevel, LaneBrightnessLevel, NoteBrightnessLevel);
}

void URhythmInputComponent::ChangeMasterVolumeStep(int32 Delta)
{
	MasterVolumeLevel = FMath::Clamp(MasterVolumeLevel + Delta, 0, 100);
	ApplyVolumes();
	SaveInputSettings();
}

void URhythmInputComponent::ChangeBGMVolumeStep(int32 Delta)
{
	BGMVolumeLevel = FMath::Clamp(BGMVolumeLevel + Delta, 0, 100);
	ApplyVolumes();
	SaveInputSettings();
}

void URhythmInputComponent::ChangeSEVolumeStep(int32 Delta)
{
	SEVolumeLevel = FMath::Clamp(SEVolumeLevel + Delta, 0, 100);
	ApplyVolumes();
	SaveInputSettings();
}

void URhythmInputComponent::ApplyVolumes()
{
	if (!SoundMix || !BGMClass || !SEClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("RhythmInput: SoundMix / BGMClass / SEClass が未設定です(BP_RhythmPlayerController で指定)"));
		return;
	}

	// 指数を変えると、スライダーの効き方が変わる
	//   1.0   = 直線(値/100 がそのまま音量)
	//   1.0超 = 低い値で急に小さくなる(2.0=2乗)
	//   1.0未満 = 低い値でも音が残る(0.5=平方根)
	const float MasterCurveExponent = 1.0f;	// マスター:
	const float BGMCurveExponent = 1.0f;	// BGM
	const float SECurveExponent = 1.0f;		// SE 補正なし

	auto ToGain = [](int32 Level, float Exponent)
		{
			const float Normalized = FMath::Clamp(Level / 100.0f, 0.0f, 1.0f);
			return FMath::Pow(Normalized, Exponent);
		};

	// マスターは BGM / SE 共通なので、同じ値を両方に掛ける
	const float Master = ToGain(MasterVolumeLevel, MasterCurveExponent);

	const float BGMGain = Master * ToGain(BGMVolumeLevel, BGMCurveExponent);
	const float SEGain = Master * ToGain(SEVolumeLevel, SECurveExponent);

	UGameplayStatics::SetBaseSoundMix(this, SoundMix);
	UGameplayStatics::SetSoundMixClassOverride(this, SoundMix, BGMClass, BGMGain, 1.0f, 0.0f, true);
	UGameplayStatics::SetSoundMixClassOverride(this, SoundMix, SEClass, SEGain, 1.0f, 0.0f, true);
}

void URhythmInputComponent::SetMasterVolumeLevel(int32 Level)
{

	MasterVolumeLevel = FMath::Clamp(Level, 0, 100);
	bVolumeDirty = true;
	ApplyVolumes();
}

void URhythmInputComponent::SetBGMVolumeLevel(int32 Level)
{
	BGMVolumeLevel = FMath::Clamp(Level, 0, 100);
	bVolumeDirty = true;
	ApplyVolumes();
}

void URhythmInputComponent::SetSEVolumeLevel(int32 Level)
{
	SEVolumeLevel = FMath::Clamp(Level, 0, 100);
	bVolumeDirty = true;
	ApplyVolumes();
}

void URhythmInputComponent::SaveVolumeSettings()
{
	bVolumeDirty = false;
	SaveInputSettings();
}

void URhythmInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// スライダーで変えた音量・明るさが未保存なら、終了時に保存する
	if (bVolumeDirty)
	{
		SaveVolumeSettings();
	}
	if (bBrightnessDirty)
	{
		SaveBrightnessSettings();
	}

	Super::EndPlay(EndPlayReason);
}

// ============================================================
// 明るさ(0〜10段階。5が標準)
// ============================================================

float URhythmInputComponent::StepToScale(int32 Step, float AtMin, float AtMid, float AtMax) const
{
	Step = FMath::Clamp(Step, BrightnessMinStep, BrightnessMaxStep);

	// 0〜5段階目: AtMin → AtMid
	if (Step <= BrightnessMidStep)
	{
		const float T = float(Step - BrightnessMinStep) / float(BrightnessMidStep - BrightnessMinStep);
		return FMath::Lerp(AtMin, AtMid, T);
	}

	// 5〜10段階目: AtMid → AtMax
	const float T = float(Step - BrightnessMidStep) / float(BrightnessMaxStep - BrightnessMidStep);
	return FMath::Lerp(AtMid, AtMax, T);
}

void URhythmInputComponent::ApplyBrightness()
{
	if (!BrightnessCollection)
	{
		UE_LOG(LogTemp, Warning, TEXT("RhythmInput: BrightnessCollection が未設定です(BP_RhythmPlayerController で指定)"));
		return;
	}

	UKismetMaterialLibrary::SetScalarParameterValue(this, BrightnessCollection, TEXT("BG"),
		StepToScale(BGBrightnessLevel, BGScaleAtMin, BGScaleAtMid, BGScaleAtMax));

	UKismetMaterialLibrary::SetScalarParameterValue(this, BrightnessCollection, TEXT("Lane"),
		StepToScale(LaneBrightnessLevel, LaneScaleAtMin, LaneScaleAtMid, LaneScaleAtMax));

	UKismetMaterialLibrary::SetScalarParameterValue(this, BrightnessCollection, TEXT("Note"),
		StepToScale(NoteBrightnessLevel, NoteScaleAtMin, NoteScaleAtMid, NoteScaleAtMax));
}

void URhythmInputComponent::SetBGBrightnessLevel(int32 Level)
{
	BGBrightnessLevel = FMath::Clamp(Level, BrightnessMinStep, BrightnessMaxStep);
	bBrightnessDirty = true;
	ApplyBrightness();
}

void URhythmInputComponent::SetLaneBrightnessLevel(int32 Level)
{
	LaneBrightnessLevel = FMath::Clamp(Level, BrightnessMinStep, BrightnessMaxStep);
	bBrightnessDirty = true;
	ApplyBrightness();
}

void URhythmInputComponent::SetNoteBrightnessLevel(int32 Level)
{
	NoteBrightnessLevel = FMath::Clamp(Level, BrightnessMinStep, BrightnessMaxStep);
	bBrightnessDirty = true;
	ApplyBrightness();
}

void URhythmInputComponent::ChangeBGBrightnessStep(int32 Delta)
{
	SetBGBrightnessLevel(BGBrightnessLevel + Delta);
	SaveBrightnessSettings();
}

void URhythmInputComponent::ChangeLaneBrightnessStep(int32 Delta)
{
	SetLaneBrightnessLevel(LaneBrightnessLevel + Delta);
	SaveBrightnessSettings();
}

void URhythmInputComponent::ChangeNoteBrightnessStep(int32 Delta)
{
	SetNoteBrightnessLevel(NoteBrightnessLevel + Delta);
	SaveBrightnessSettings();
}

void URhythmInputComponent::SaveBrightnessSettings()
{
	bBrightnessDirty = false;
	SaveInputSettings();
}