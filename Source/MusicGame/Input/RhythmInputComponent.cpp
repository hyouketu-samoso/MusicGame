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

	// ポーズ中はレーン入力を出さない
	if(GetWorld()->IsPaused()) return;

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
			HeldKeys.Remove(Pair.Key);
		}
	}
	
	// スティック
	const double Now = GetWorld()->GetTimeSeconds();
	const float LX = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float RX = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX);

	UpdateStick(LX, LeftStick, 0, 1, Now);	// L ← =0, L → =1
	UpdateStick(RX, RightStick, 2, 3, Now);	// R ← =2, R → =3

	if (bShowDebug && GEngine)
	{
		// キー(Key)を固定にすると、同じ行が毎フレーム上書きされる
		GEngine->AddOnScreenDebugMessage(100, 0.f, FColor::Cyan,
			FString::Printf(TEXT("LX=%d t =%.3f count=%d"),
				LastLane, LastLaneTime, FireCount));
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

	// ニュートラルに戻ったら再受付
	if (AbsX < ReleaseThreshold)
	{
		State.bArmed = true;
		return;
	}

	if (!State.bArmed || AbsX < FireThreshold)return;

	const int32 Dir = (X < 0.f) ? -1 : 1;

	// 発火直後の逆方向(戻りの勢い)は無視
	if (State.LastDir != 0 && Dir != State.LastDir
		&& (Now - State.LastFireTime) < ReverseLockSec)
	{
		return;
	}

	State.bArmed = false;
	State.LastDir = Dir;
	State.LastFireTime = Now;

	FireLane(Dir < 0 ? LeftLane : RightLane, EInputType::Flick);
}


void URhythmInputComponent::FireLane(int32 Lane, EInputType Type)
{
	FLaneInput In;
	In.Lane = Lane;
	In.Timestamp = GetWorld()->GetTimeSeconds();	//後で楽曲時間にする
	In.InputType = Type;

	UE_LOG(LogTemp, Log, TEXT("Lane %d t = %.3f type=%s"), Lane, In.Timestamp,
		Type == EInputType::Flick ? TEXT("Flick"):TEXT("Key"));

	LastLane = Lane;
	LastLaneTime = In.Timestamp;
	++FireCount;

	OnLaneInput.Broadcast(In);
}