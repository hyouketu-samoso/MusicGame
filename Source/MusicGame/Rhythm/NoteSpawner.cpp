#include "NoteSpawner.h"
#include "NoteActor.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

ANoteSpawner::ANoteSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

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

	if (bAutoSpawn)
	{
		GetWorldTimerManager().SetTimer(AutoSpawnTimer, this, &ANoteSpawner::SpawnWave, SpawnInterval, true, 0.5f);
	}
}

void ANoteSpawner::SpawnWave()
{
	for (int32 Lane = 0; Lane < LaneColors.Num(); ++Lane)
	{
		SpawnNote(Lane);
	}
}

void ANoteSpawner::SpawnNote(int32 LaneIndex)
{
	if (!NoteClass || !LaneColors.IsValidIndex(LaneIndex))
	{
		return;
	}

	// 全レーン共通の出現地点（奥の中央）
	const FVector StartPos = GetActorLocation()
		+ GetActorForwardVector() * SpawnDistance
		+ FVector::UpVector * SpawnHeight;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (ANoteActor* Note = GetWorld()->SpawnActor<ANoteActor>(NoteClass, StartPos, FRotator::ZeroRotator, Params))
	{
		Note->Launch(StartPos, GetLaneTarget(LaneIndex), ArcHeight, TravelTime, LaneColors[LaneIndex]);
	}
}

FVector ANoteSpawner::GetLaneTarget(int32 LaneIndex) const
{
	// レーンを中央揃えで左右に並べる（-2, -1, 0, 1, 2）
	const float Offset = (LaneIndex - (LaneColors.Num() - 1) * 0.5f) * LaneSpacing;
	return GetActorLocation() + GetActorRightVector() * Offset;
}
