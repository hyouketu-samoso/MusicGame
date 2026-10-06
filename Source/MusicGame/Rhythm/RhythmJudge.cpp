#include "RhythmJudge.h"
#include "NoteActor.h"
#include "HoldNoteActor.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

ARhythmJudge::ARhythmJudge()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// ThirdPersonの操作（WASD・Space・マウス）とぶつからないキーを仮置き
	LaneKeys = { EKeys::F, EKeys::G, EKeys::H, EKeys::J};

	JudgementCounts.Init(0, StaticEnum<ERhythmJudgement>()->NumEnums() - 1);
}

void ARhythmJudge::BeginPlay()
{
	Super::BeginPlay();

	// プレイヤーのキー入力をこのActorでも受け取れるようにする
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		EnableInput(PC);

		for (int32 Lane = 0; Lane < LaneKeys.Num(); ++Lane)
		{
			// キーごとに「どのレーンか」を付けて PressLane / ReleaseLane に繋ぐ
			FInputKeyBinding PressBinding(FInputChord(LaneKeys[Lane]), IE_Pressed);
			PressBinding.KeyDelegate.GetDelegateForManualSet().BindUObject(this, &ARhythmJudge::PressLane, Lane);
			InputComponent->KeyBindings.Add(PressBinding);

			FInputKeyBinding ReleaseBinding(FInputChord(LaneKeys[Lane]), IE_Released);
			ReleaseBinding.KeyDelegate.GetDelegateForManualSet().BindUObject(this, &ARhythmJudge::ReleaseLane, Lane);
			InputComponent->KeyBindings.Add(ReleaseBinding);
		}
	}
}

void ARhythmJudge::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const double Now = GetWorld()->GetTimeSeconds();

	// 押されないまま BadWindow を過ぎたノーツはミス
	for (int32 i = ActiveNotes.Num() - 1; i >= 0; --i)
	{
		ANoteActor* Note = ActiveNotes[i];
		if (!IsValid(Note))
		{
			ActiveNotes.RemoveAt(i);
			continue;
		}

		if (Now - Note->GetHitTime() > BadWindow)
		{
			ActiveNotes.RemoveAt(i);

			if (Note->IsHold())
			{
				// ホールドは始点を逃したら終点もミス
				ApplyJudgement(Note, ERhythmJudgement::Miss, 0.0f, ENoteJudgePoint::HoldStart);
				ApplyJudgement(Note, ERhythmJudgement::Miss, 0.0f, ENoteJudgePoint::HoldEnd);
			}
			else
			{
				ApplyJudgement(Note, ERhythmJudgement::Miss, 0.0f, ENoteJudgePoint::Tap);
			}
		}
	}

	// 終点を BadWindow 過ぎても離さなかったホールドはミス
	TArray<int32> OverdueLanes;
	for (const TPair<int32, TObjectPtr<AHoldNoteActor>>& Pair : HoldingNotes)
	{
		if (!IsValid(Pair.Value) || Now - Pair.Value->GetEndTime() > BadWindow)
		{
			OverdueLanes.Add(Pair.Key);
		}
	}
	for (int32 Lane : OverdueLanes)
	{
		AHoldNoteActor* Hold = HoldingNotes.FindAndRemoveChecked(Lane);
		if (IsValid(Hold))
		{
			ApplyJudgement(Hold, ERhythmJudgement::Miss, 0.0f, ENoteJudgePoint::HoldEnd);
		}
	}
}

void ARhythmJudge::RegisterNote(ANoteActor* Note)
{
	if (IsValid(Note))
	{
		ActiveNotes.Add(Note);
		Note->SetWaitForJudge(true);
	}
}

void ARhythmJudge::PressLane(int32 LaneIndex)
{
	// ホールド中のレーンは押し直せない
	if (HoldingNotes.Contains(LaneIndex))
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	// このレーンで判定幅に入っているノーツのうち、一番先に届くものを対象にする
	int32 TargetIndex = INDEX_NONE;
	for (int32 i = 0; i < ActiveNotes.Num(); ++i)
	{
		const ANoteActor* Note = ActiveNotes[i];
		if (!IsValid(Note) || Note->GetLaneIndex() != LaneIndex)
		{
			continue;
		}

		const float Error = static_cast<float>(Now - Note->GetHitTime());
		if (FMath::Abs(Error) > BadWindow)
		{
			continue;
		}

		if (TargetIndex == INDEX_NONE || Note->GetHitTime() < ActiveNotes[TargetIndex]->GetHitTime())
		{
			TargetIndex = i;
		}
	}

	// 判定幅に何も無いときの空押しは無視する
	if (TargetIndex == INDEX_NONE)
	{
		return;
	}

	ANoteActor* Note = ActiveNotes[TargetIndex];
	ActiveNotes.RemoveAt(TargetIndex);

	const float Error = static_cast<float>(Now - Note->GetHitTime());
	const ERhythmJudgement Judgement = CalcJudgement(FMath::Abs(Error));

	if (AHoldNoteActor* Hold = Cast<AHoldNoteActor>(Note))
	{
		// ホールドは始点の判定を出して、離すまで押し続けの状態にする
		ApplyJudgement(Hold, Judgement, Error, ENoteJudgePoint::HoldStart);
		HoldingNotes.Add(LaneIndex, Hold);
	}
	else
	{
		ApplyJudgement(Note, Judgement, Error, ENoteJudgePoint::Tap);
	}
}

void ARhythmJudge::ReleaseLane(int32 LaneIndex)
{
	TObjectPtr<AHoldNoteActor> Hold;
	if (!HoldingNotes.RemoveAndCopyValue(LaneIndex, Hold) || !IsValid(Hold))
	{
		return;
	}

	// 終点に届く時刻とのズレで判定する（押したときと同じ幅）。早すぎればミス
	// 遅すぎる場合は離す前に Tick でミスになっている
	const float Error = static_cast<float>(GetWorld()->GetTimeSeconds() - Hold->GetEndTime());
	const ERhythmJudgement Judgement = CalcJudgement(FMath::Abs(Error));

	ApplyJudgement(Hold, Judgement, (Judgement == ERhythmJudgement::Miss) ? 0.0f : Error, ENoteJudgePoint::HoldEnd);
}

ERhythmJudgement ARhythmJudge::CalcJudgement(float AbsError) const
{
	if (AbsError <= PerfectWindow) { return ERhythmJudgement::Perfect; }
	if (AbsError <= GreatWindow)   { return ERhythmJudgement::Great; }
	if (AbsError <= GoodWindow)    { return ERhythmJudgement::Good; }
	if (AbsError <= BadWindow)     { return ERhythmJudgement::Bad; }
	return ERhythmJudgement::Miss;
}

void ARhythmJudge::ApplyJudgement(ANoteActor* Note, ERhythmJudgement Judgement, float TimingError, ENoteJudgePoint Point)
{
	// グッド以上でコンボ継続、バット以下で途切れる
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

	const int32 LaneIndex = Note->GetLaneIndex();

	Note->OnJudged(Judgement, Point);

	OnNoteJudged.Broadcast(Judgement, LaneIndex, TimingError, Combo, Point);

	if (bShowDebugText && GEngine)
	{
		static const FColor Colors[] = { FColor::Yellow, FColor::Cyan, FColor::Green, FColor::Orange, FColor::Red };
		static const TCHAR* PointNames[] = { TEXT(""), TEXT("[HOLD START] "), TEXT("[HOLD END] ") };

		const FString Name = StaticEnum<ERhythmJudgement>()->GetNameStringByValue(static_cast<int64>(Judgement));
		const FString Timing = (Judgement == ERhythmJudgement::Miss) ? TEXT("") : FString::Printf(TEXT("  (%+.0fms)"), TimingError * 1000.0f);

		// キー 1 番を使い回して、最新の判定だけを表示する
		GEngine->AddOnScreenDebugMessage(1, 1.0f, Colors[static_cast<int32>(Judgement)],
			FString::Printf(TEXT("%s%s%s   Combo %d"), PointNames[static_cast<int32>(Point)], *Name.ToUpper(), *Timing, Combo), true, FVector2D(2.0f, 2.0f));
	}
}

void ARhythmJudge::ResetResult()
{
	Combo = 0;
	MaxCombo = 0;
	for (int32& Count : JudgementCounts)
	{
		Count = 0;
	}
}

int32 ARhythmJudge::GetJudgementCount(ERhythmJudgement Judgement) const
{
	const int32 Index = static_cast<int32>(Judgement);
	return JudgementCounts.IsValidIndex(Index) ? JudgementCounts[Index] : 0;
}
