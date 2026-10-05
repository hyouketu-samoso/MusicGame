#include "NoteSpawner.h"
#include "NoteActor.h"
#include "HoldNoteActor.h"
#include "RhythmJudge.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

ANoteSpawner::ANoteSpawner()
{
	// 判定ラインの描画のためにTickを使う
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(RootComponent);
		Arrow->ArrowSize = 3.0f;
	}
#endif

	NoteClass = ANoteActor::StaticClass();
	HoldNoteClass = AHoldNoteActor::StaticClass();

	// 緑・青・赤・黄・紫 の5レーン
	LaneColors = {
		FLinearColor::Green,
		FLinearColor::Blue,
		FLinearColor::Red,
		FLinearColor::Yellow,
		FLinearColor(0.6f, 0.0f, 1.0f),
	};
}

void ANoteSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 判定役が指定されていなければレベル内から探し、それでも無ければ作る
	if (!Judge)
	{
		Judge = Cast<ARhythmJudge>(UGameplayStatics::GetActorOfClass(this, ARhythmJudge::StaticClass()));
	}
	if (!Judge)
	{
		Judge = GetWorld()->SpawnActor<ARhythmJudge>(GetActorLocation(), GetActorRotation());
	}

	if (bAutoSpawn)
	{
		GetWorldTimerManager().SetTimer(AutoSpawnTimer, this, &ANoteSpawner::AutoSpawn, SpawnInterval, true, 0.5f);
	}
}

void ANoteSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowJudgeLine)
	{
		DrawJudgeLine();
	}
}

void ANoteSpawner::AutoSpawn()
{
	switch (SpawnPattern)
	{
	case ENoteSpawnPattern::AllLanes:
		SpawnWave();
		break;

	case ENoteSpawnPattern::RandomLane:
		SpawnRandomNote();
		break;
	}
}

void ANoteSpawner::SpawnRandomNote()
{
	// 前のノーツ（ホールド中のレーンなど）と重ならないレーンから選ぶ
	TArray<int32> FreeLanes;
	for (int32 Lane = 0; Lane < LaneColors.Num(); ++Lane)
	{
		if (IsLaneFree(Lane))
		{
			FreeLanes.Add(Lane);
		}
	}
	if (FreeLanes.Num() == 0)
	{
		return;
	}

	const int32 Lane = FreeLanes[FMath::RandRange(0, FreeLanes.Num() - 1)];

	if (FMath::FRand() < HoldChance)
	{
		SpawnHoldNote(Lane, FMath::FRandRange(HoldDurationMin, FMath::Max(HoldDurationMin, HoldDurationMax)));
	}
	else
	{
		SpawnNote(Lane);
	}
}

void ANoteSpawner::SpawnWave()
{
	for (int32 Lane = 0; Lane < LaneColors.Num(); ++Lane)
	{
		if (IsLaneFree(Lane))
		{
			SpawnNote(Lane);
		}
	}
}

void ANoteSpawner::SpawnNote(int32 LaneIndex)
{
	SpawnNoteInternal(NoteClass, LaneIndex, 0.0f);
}

void ANoteSpawner::SpawnHoldNote(int32 LaneIndex, float HoldDuration)
{
	SpawnNoteInternal(HoldNoteClass, LaneIndex, HoldDuration);
}

bool ANoteSpawner::IsLaneFree(int32 LaneIndex) const
{
	if (!LaneBusyUntil.IsValidIndex(LaneIndex))
	{
		return true;
	}

	// 今出すノーツが届く時刻が、前のノーツが届く時刻 + MinLaneGap より後なら空いている
	const double NewHitTime = GetWorld()->GetTimeSeconds() + TravelTime;
	return NewHitTime >= LaneBusyUntil[LaneIndex] + MinLaneGap;
}

void ANoteSpawner::SpawnNoteInternal(TSubclassOf<ANoteActor> Class, int32 LaneIndex, float HoldDuration)
{
	if (!Class || !LaneColors.IsValidIndex(LaneIndex))
	{
		return;
	}

	// 全レーン共通の出現地点（奥の中央）
	const FVector StartPos = GetActorLocation()
		+ GetActorForwardVector() * SpawnDistance
		+ FVector::UpVector * SpawnHeight;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANoteActor* Note = GetWorld()->SpawnActor<ANoteActor>(Class, StartPos, FRotator::ZeroRotator, Params);
	if (!Note)
	{
		return;
	}

	// ホールドの長さは Launch より前に決めておく
	AHoldNoteActor* Hold = Cast<AHoldNoteActor>(Note);
	if (Hold)
	{
		Hold->SetHoldDuration(HoldDuration);
	}

	Note->Launch(StartPos, GetLaneTarget(LaneIndex), ArcHeight, TravelTime, LaneColors[LaneIndex], LaneIndex);

	if (Judge)
	{
		Judge->RegisterNote(Note);
	}

	// このレーンが埋まっている時刻を記録する
	if (LaneBusyUntil.Num() < LaneColors.Num())
	{
		LaneBusyUntil.SetNumZeroed(LaneColors.Num());
	}
	LaneBusyUntil[LaneIndex] = Hold ? Hold->GetEndTime() : Note->GetHitTime();
}

FVector ANoteSpawner::GetLaneTarget(int32 LaneIndex) const
{
	// レーンを中央揃えで左右に並べる（-2, -1, 0, 1, 2）
	const float Offset = (LaneIndex - (LaneColors.Num() - 1) * 0.5f) * LaneSpacing;
	return GetActorLocation() + GetActorRightVector() * Offset;
}

void ANoteSpawner::DrawJudgeLine() const
{
	const int32 LaneNum = LaneColors.Num();
	if (LaneNum == 0)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const FVector Right = GetActorRightVector();
	const FVector Up = FVector::UpVector;

	// 端のレーンから端のレーンまで横に1本線を引く
	const FVector LineStart = GetLaneTarget(0) - Right * LaneSpacing * 0.5f;
	const FVector LineEnd = GetLaneTarget(LaneNum - 1) + Right * LaneSpacing * 0.5f;
	DrawDebugLine(World, LineStart, LineEnd, FColor::White, false, -1.0f, 0, 2.0f);

	// 各レーンの判定位置に、こちら（カメラ側）を向いた円を描く。
	// ノーツがこの円にぴったり重なった瞬間がパーフェクト
	for (int32 Lane = 0; Lane < LaneNum; ++Lane)
	{
		DrawDebugCircle(World, GetLaneTarget(Lane), JudgeCircleRadius, 32, LaneColors[Lane].ToFColor(true), false, -1.0f, 0, 3.0f, Right, Up, false);
	}
}
