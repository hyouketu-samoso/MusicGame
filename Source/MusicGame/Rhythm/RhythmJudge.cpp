#include "RhythmJudge.h"
#include "NoteActor.h"
#include "HoldNoteActor.h"
#include "RhythmScoreComponent.h"
#include "Input/RhythmInputComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

ARhythmJudge::ARhythmJudge()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 4レーン：左外側 / 左内側 / 右内側 / 右外側（入力コンポーネントと同じ割り当て）
	LaneKeys = { EKeys::D, EKeys::F, EKeys::J, EKeys::K };

	ScoreComponent = CreateDefaultSubobject<URhythmScoreComponent>(TEXT("Score"));
}

void ARhythmJudge::BeginPlay()
{
	Super::BeginPlay();

	ScoreComponent->OnGameOver.AddDynamic(this, &ARhythmJudge::HandleGameOver);

	ShowDebugStatus();

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	// 入力コンポーネント（キーボード・スティック弾き → 4レーン）があれば、押した・離したはそこから受け取る。
	// キーの割り当てや入力時刻は入力コンポーネント側に一本化し、ここでは自分でキーを受け取らない
	if (URhythmInputComponent* RhythmInput = PC->FindComponentByClass<URhythmInputComponent>())
	{
		RhythmInput->OnLaneInput.AddDynamic(this, &ARhythmJudge::HandleLaneInput);
		RhythmInput->OnLaneRelease.AddDynamic(this, &ARhythmJudge::HandleLaneRelease);
		return;
	}

	// 入力コンポーネントが無いときだけ、LaneKeys を自分で受け取る
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

void ARhythmJudge::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsGameOver())
	{
		return;
	}

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

			// このミスでゲームオーバーになったら、残りは HandleGameOver で片付け済み
			if (IsGameOver())
			{
				return;
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
		TObjectPtr<AHoldNoteActor> Hold;
		if (HoldingNotes.RemoveAndCopyValue(Lane, Hold) && IsValid(Hold))
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
	PressLaneAt(LaneIndex, GetWorld()->GetTimeSeconds());
}

void ARhythmJudge::PressLaneAt(int32 LaneIndex, double InputTime)
{
	// ゲームオーバー後と、ホールド中のレーンは受け付けない
	// （ポーズ明けにホールド中のレーンを押し直した場合も、ここでそのままホールド継続になる）
	if (IsGameOver() || HoldingNotes.Contains(LaneIndex))
	{
		return;
	}

	const double Now = InputTime;

	// 候補：同じレーン・未判定・−BadWindow ≤ Δt ≤ +BadWindow（Δt = 入力時刻 − ノーツの判定時刻）
	// 対象：候補のうち |Δt| が最小のノーツ。|Δt| が同じなら判定時刻が早いノーツ（入力判定仕様書「判定仕様」）
	int32 TargetIndex = INDEX_NONE;
	double TargetAbsError = 0.0;
	for (int32 i = 0; i < ActiveNotes.Num(); ++i)
	{
		const ANoteActor* Note = ActiveNotes[i];
		if (!IsValid(Note) || Note->GetLaneIndex() != LaneIndex)
		{
			continue;
		}

		const double AbsError = FMath::Abs(Now - Note->GetHitTime());
		if (AbsError > BadWindow)
		{
			continue;
		}

		const bool bCloser = AbsError < TargetAbsError;
		const bool bSameButEarlier = (AbsError == TargetAbsError) && Note->GetHitTime() < ActiveNotes[TargetIndex]->GetHitTime();
		if (TargetIndex == INDEX_NONE || bCloser || bSameButEarlier)
		{
			TargetIndex = i;
			TargetAbsError = AbsError;
		}
	}

	// 候補なしは空打ち。ノーツ・スコア・コンボ・体力は変えない（Δt < −BadWindow の早すぎる入力もここ）
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

		// 始点の Bad でゲームオーバーになった場合は、押し続けにせず手放す
		if (IsGameOver())
		{
			Hold->Abandon();
		}
		else
		{
			HoldingNotes.Add(LaneIndex, Hold);
		}
	}
	else
	{
		ApplyJudgement(Note, Judgement, Error, ENoteJudgePoint::Tap);
	}
}

void ARhythmJudge::ReleaseLane(int32 LaneIndex)
{
	ReleaseLaneAt(LaneIndex, GetWorld()->GetTimeSeconds());
}

void ARhythmJudge::ReleaseLaneAt(int32 LaneIndex, double InputTime)
{
	TObjectPtr<AHoldNoteActor> Hold;
	if (!HoldingNotes.RemoveAndCopyValue(LaneIndex, Hold) || !IsValid(Hold))
	{
		return;
	}

	// 終点に届く時刻とのズレで判定する（押したときと同じ幅）。早すぎればミス
	// 遅すぎる場合は離す前に Tick でミスになっている
	const float Error = static_cast<float>(InputTime - Hold->GetEndTime());
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
	// ゲームオーバー後に来た判定（ホールドの終点ミスなど）は反映せず、ノーツを手放すだけ
	if (IsGameOver())
	{
		Note->Abandon();
		return;
	}

	const int32 LaneIndex = Note->GetLaneIndex();

	Note->OnJudged(Judgement, Point);

	// コンボ → スコア → 体力 の順に更新。体力が 0 になるとここで HandleGameOver が呼ばれる
	ScoreComponent->AddJudgement(Judgement);

	const int32 Combo = ScoreComponent->GetCombo();

	OnNoteJudged.Broadcast(Judgement, LaneIndex, TimingError, Combo, Point);

	ShowDebugStatus();

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

void ARhythmJudge::HandleGameOver()
{
	// 判定待ち・ホールド中のノーツを手放す（そのまま飛んでいって消える）
	for (ANoteActor* Note : ActiveNotes)
	{
		if (IsValid(Note))
		{
			Note->Abandon();
		}
	}
	ActiveNotes.Reset();

	for (const TPair<int32, TObjectPtr<AHoldNoteActor>>& Pair : HoldingNotes)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->Abandon();
		}
	}
	HoldingNotes.Reset();

	if (bShowDebugText && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3, 10.0f, FColor::Red, TEXT("GAME OVER"), true, FVector2D(4.0f, 4.0f));
	}
}

void ARhythmJudge::HandleLaneInput(const FLaneInput& Input)
{
	// 入力時刻は入力コンポーネントが記録したものを使う（楽曲時間への差し替えも入力側の1か所で済む）
	PressLaneAt(Input.Lane, Input.Timestamp);
}

void ARhythmJudge::HandleLaneRelease(const FLaneInput& Input)
{
	// ポーズすると入力コンポーネントは押していたレーンをすべて「離した」扱いにする。
	// これで終点が判定されないよう、ポーズ中の離しは無視してホールドを続ける
	if (GetWorld()->IsPaused())
	{
		return;
	}

	ReleaseLaneAt(Input.Lane, Input.Timestamp);
}

void ARhythmJudge::ShowDebugStatus() const
{
	if (!bShowDebugText || !GEngine)
	{
		return;
	}

	// キー 2 番を使い回して、スコアと体力を常に表示する
	GEngine->AddOnScreenDebugMessage(2, 3600.0f, FColor::White,
		FString::Printf(TEXT("SCORE %.2f   HP %.0f / %.0f   MAX COMBO %d"),
			ScoreComponent->GetDisplayScore(), ScoreComponent->GetHealth(), ScoreComponent->GetMaxHealth(), ScoreComponent->GetMaxCombo()),
		true, FVector2D(1.5f, 1.5f));
}

void ARhythmJudge::ResetResult()
{
	ScoreComponent->ResetResult();
	ShowDebugStatus();
}

int32 ARhythmJudge::GetCombo() const
{
	return ScoreComponent->GetCombo();
}

int32 ARhythmJudge::GetMaxCombo() const
{
	return ScoreComponent->GetMaxCombo();
}

int32 ARhythmJudge::GetJudgementCount(ERhythmJudgement Judgement) const
{
	return ScoreComponent->GetJudgementCount(Judgement);
}

bool ARhythmJudge::IsGameOver() const
{
	return ScoreComponent->IsGameOver();
}
