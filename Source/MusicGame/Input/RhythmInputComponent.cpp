#include "RhythmInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "InputCoreTypes.h"

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
		ReleaseAllLanes();
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