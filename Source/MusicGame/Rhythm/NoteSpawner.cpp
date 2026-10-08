#include "NoteSpawner.h"

#include "NoteActor.h"
#include "HoldNoteActor.h"
#include "RhythmJudge.h"
#include "ChartImporter.h"

#include "Components/ArrowComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

#include "GameFramework/PlayerController.h"

#include "Kismet/GameplayStatics.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "Misc/Paths.h"


// ============================================================
// Constructor
// ============================================================

ANoteSpawner::ANoteSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("Root")
		);

#if WITH_EDITORONLY_DATA

	Arrow =
		CreateEditorOnlyDefaultSubobject<UArrowComponent>(
			TEXT("Arrow")
		);

	if (Arrow)
	{
		Arrow->SetupAttachment(RootComponent);
		Arrow->ArrowSize = 3.0f;
	}

#endif


	// ========================================================
	// Camera
	// ========================================================

	Camera =
		CreateDefaultSubobject<UCameraComponent>(
			TEXT("Camera")
		);

	Camera->SetupAttachment(RootComponent);

	UpdateCameraTransform();


	// ========================================================
	// Note Class
	// ========================================================

	NoteClass =
		ANoteActor::StaticClass();

	HoldNoteClass =
		AHoldNoteActor::StaticClass();


	// ========================================================
	// 4 Lanes
	// ========================================================

	LaneColors =
	{
		FLinearColor(0.0f, 0.9f, 1.0f),
		FLinearColor(0.1f, 0.3f, 1.0f),
		FLinearColor(1.0f, 0.2f, 0.8f),
		FLinearColor(1.0f, 0.85f, 0.1f)
	};


	// ========================================================
	// Lane Target
	// ========================================================

	LaneTargetOffsets =
	{
		FVector(-60.0f, -300.0f, 0.0f),
		FVector(-205.0f, -90.0f, 0.0f),
		FVector(-205.0f, 90.0f, 0.0f),
		FVector(-60.0f, 300.0f, 0.0f)
	};
}


// ============================================================
// BeginPlay
// ============================================================

void ANoteSpawner::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("========== NoteSpawner BeginPlay ==========")
	);


	// ========================================================
	// Judge取得
	// ========================================================

	if (!Judge)
	{
		Judge =
			Cast<ARhythmJudge>(
				UGameplayStatics::GetActorOfClass(
					this,
					ARhythmJudge::StaticClass()
				)
			);
	}

	if (!Judge)
	{
		Judge =
			GetWorld()->SpawnActor<ARhythmJudge>(
				GetActorLocation(),
				GetActorRotation()
			);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"NoteSpawner: RhythmJudgeを自動生成しました"
			)
		);
	}


	// ========================================================
	// Chart Mode
	// ========================================================

	if (bUseChart)
	{
		// Chartモードではランダム生成を絶対に行わない
		bAutoSpawn = false;

		LoadChart();

		if (bChartLoaded)
		{
			StartChart();
		}
	}


	// ========================================================
	// Random Test Mode
	// ========================================================

	if (bAutoSpawn && !bUseChart)
	{
		GetWorldTimerManager().SetTimer(
			AutoSpawnTimer,
			this,
			&ANoteSpawner::AutoSpawn,
			SpawnInterval,
			true,
			0.5f
		);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"NoteSpawner: Random Test Spawn ON"
			)
		);
	}


	// ========================================================
	// Camera
	// ========================================================

	if (bAutoCamera)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			this,
			&ANoteSpawner::ActivateAutoCamera
		);
	}
}


// ============================================================
// OnConstruction
// ============================================================

void ANoteSpawner::OnConstruction(
	const FTransform& Transform
)
{
	Super::OnConstruction(Transform);

	UpdateCameraTransform();
}


// ============================================================
// Camera Transform
// ============================================================

void ANoteSpawner::UpdateCameraTransform()
{
	if (!Camera)
	{
		return;
	}

	Camera->SetRelativeLocationAndRotation(
		FVector(
			-CameraDistance,
			0.0f,
			CameraHeight
		),
		FRotator(
			CameraPitch,
			0.0f,
			0.0f
		)
	);

	Camera->SetFieldOfView(
		CameraFieldOfView
	);
}


// ============================================================
// Activate Camera
// ============================================================

void ANoteSpawner::ActivateAutoCamera()
{
	if (!GetWorld())
	{
		return;
	}

	APlayerController* PC =
		GetWorld()->GetFirstPlayerController();

	if (!PC)
	{
		return;
	}

	PC->bAutoManageActiveCameraTarget = false;

	PC->SetViewTarget(this);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"NoteSpawner: Auto Camera Activated"
		)
	);
}


// ============================================================
// Tick
// ============================================================
void ANoteSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowJudgeLine)
	{
		DrawJudgeLine();
	}

	if (!bUseChart || !bChartRunning)
	{
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	const double Now =
		GetWorld()->GetTimeSeconds();

	const float CurrentSongTime =
		static_cast<float>(
			Now -
			ChartStartWorldTime
			);

	if (!bChartPlaying &&
		CurrentSongTime >= 0.0f)
	{
		bChartPlaying = true;

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("========== SONG START ==========")
		);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SongTime = 0.000")
		);
	}

	UpdateChartSpawn(CurrentSongTime);
}

// ============================================================
// Load Chart
// ============================================================

void ANoteSpawner::LoadChart()
{
	if (!GetWorld())
	{
		return;
	}

	const FString FullPath =
		FPaths::Combine(
			FPaths::ProjectContentDir(),
			ChartFilePath
		);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("NoteSpawner: Chart loading: %s"),
		*FullPath
	);

	UChartImporter* Importer =
		NewObject<UChartImporter>();

	if (!Importer)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("NoteSpawner: ChartImporter生成失敗")
		);

		return;
	}

	ChartNotes.Empty();

	bChartLoaded =
		Importer->LoadChart(
			FullPath,
			ChartNotes
		);

	if (!bChartLoaded)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("NoteSpawner: Chart読み込み失敗: %s"),
			*FullPath
		);

		return;
	}

	ChartNotes.Sort(
		[](const FNoteData& A, const FNoteData& B)
		{
			return A.Time < B.Time;
		}
	);

	NextChartNoteIndex = 0;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"NoteSpawner: Chart読み込み成功 "
			"Notes=%d"
		),
		ChartNotes.Num()
	);

	for (int32 i = 0; i < ChartNotes.Num(); ++i)
	{
		const FNoteData& Note =
			ChartNotes[i];

		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"Chart[%d] Time=%.3f Lane=%d "
				"Type=%s Duration=%.3f"
			),
			i,
			Note.Time,
			Note.Lane,
			*Note.Type,
			Note.Duration
		);
	}
}

// ============================================================
// Start Chart
// ============================================================

void ANoteSpawner::StartChart()
{
	if (!bChartLoaded)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"NoteSpawner: Chartが読み込まれていません"
			)
		);

		return;
	}

	if (!GetWorld())
	{
		return;
	}

	const double Now =
		GetWorld()->GetTimeSeconds();

	ChartStartWorldTime =
		Now +
		static_cast<double>(
			TravelTime
			);

	NextChartNoteIndex = 0;

	bChartRunning = true;
	bChartPlaying = false;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("========== CHART START ==========")
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"ChartStartWorldTime = %.3f"
		),
		ChartStartWorldTime
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"TravelTime = %.3f"
		),
		TravelTime
	);
}
// ============================================================
// Update Chart Spawn
// ============================================================

void ANoteSpawner::UpdateChartSpawn(
	float CurrentSongTime
)
{
	if (!bChartLoaded)
	{
		return;
	}

	while (
		NextChartNoteIndex <
		ChartNotes.Num()
		)
	{
		const FNoteData& NoteData =
			ChartNotes[
				NextChartNoteIndex
			];

		// ノーツ生成時刻
		const float SpawnTime =
			NoteData.Time -
			TravelTime;

		// まだ生成時刻ではない
		if (CurrentSongTime < SpawnTime)
		{
			break;
		}

		const int32 LaneCount =
			LaneColors.Num();

		if (LaneCount <= 0)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("NoteSpawner: LaneColorsが空です")
			);

			return;
		}

		const int32 Lane =
			FMath::Clamp(
				NoteData.Lane,
				0,
				LaneCount - 1
			);

		TSubclassOf<ANoteActor> SpawnClass =
			NoteClass;

		float HoldDuration = 0.0f;

		if (
			NoteData.Type.Equals(
				TEXT("hold"),
				ESearchCase::IgnoreCase
			)
			)
		{
			SpawnClass =
				HoldNoteClass;

			HoldDuration =
				FMath::Max(
					NoteData.Duration,
					0.0f
				);
		}
		else if (
			NoteData.Type.Equals(
				TEXT("flick"),
				ESearchCase::IgnoreCase
			)
			)
		{
			SpawnClass =
				NoteClass;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"CHART SPAWN "
				"Index=%d "
				"SongTime=%.3f "
				"NoteTime=%.3f "
				"SpawnTime=%.3f "
				"Lane=%d"
			),
			NextChartNoteIndex,
			CurrentSongTime,
			NoteData.Time,
			SpawnTime,
			Lane
		);

		SpawnNoteInternal(
			SpawnClass,
			Lane,
			HoldDuration
		);

		// 次のノーツへ
		NextChartNoteIndex =
			NextChartNoteIndex + 1;
	}
}
// ============================================================
// Random Note
// ============================================================

void ANoteSpawner::SpawnRandomNote()
{
	if (bUseChart)
	{
		return;
	}


	TArray<int32> FreeLanes;


	for (
		int32 Lane = 0;
		Lane < LaneColors.Num();
		++Lane
		)
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


	const int32 Lane =
		FreeLanes[
			FMath::RandRange(
				0,
				FreeLanes.Num() - 1
			)
		];


	if (FMath::FRand() < HoldChance)
	{
		SpawnHoldNote(
			Lane,
			FMath::FRandRange(
				HoldDurationMin,
				FMath::Max(
					HoldDurationMin,
					HoldDurationMax
				)
			)
		);
	}
	else
	{
		SpawnNote(Lane);
	}
}


// ============================================================
// Spawn Wave
// ============================================================

void ANoteSpawner::SpawnWave()
{
	if (bUseChart)
	{
		return;
	}


	for (
		int32 Lane = 0;
		Lane < LaneColors.Num();
		++Lane
		)
	{
		if (IsLaneFree(Lane))
		{
			SpawnNote(Lane);
		}
	}
}


// ============================================================
// Spawn Note
// ============================================================

void ANoteSpawner::SpawnNote(
	int32 LaneIndex
)
{
	SpawnNoteInternal(
		NoteClass,
		LaneIndex,
		0.0f
	);
}


// ============================================================
// Spawn Hold
// ============================================================

void ANoteSpawner::SpawnHoldNote(
	int32 LaneIndex,
	float HoldDuration
)
{
	SpawnNoteInternal(
		HoldNoteClass,
		LaneIndex,
		HoldDuration
	);
}


// ============================================================
// Is Lane Free
// ============================================================

bool ANoteSpawner::IsLaneFree(
	int32 LaneIndex
) const
{
	if (!GetWorld())
	{
		return true;
	}

	if (!LaneBusyUntil.IsValidIndex(LaneIndex))
	{
		return true;
	}


	const double NewHitTime =
		GetWorld()->GetTimeSeconds()
		+
		TravelTime;


	return
		NewHitTime >=
		LaneBusyUntil[LaneIndex];
}


// ============================================================
// Spawn Note Internal
// ============================================================

void ANoteSpawner::SpawnNoteInternal(
	TSubclassOf<ANoteActor> Class,
	int32 LaneIndex,
	float HoldDuration
)
{
	if (!GetWorld())
	{
		return;
	}


	if (!Class)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"NoteSpawner: Note Classがありません"
			)
		);

		return;
	}


	if (!LaneColors.IsValidIndex(LaneIndex))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"NoteSpawner: LaneIndex=%d が不正です"
			),
			LaneIndex
		);

		return;
	}


	// ========================================================
	// 出現位置
	// ========================================================

	const FVector StartPos =
		GetActorLocation()
		+
		GetActorForwardVector()
		*
		SpawnDistance
		+
		FVector::UpVector
		*
		SpawnHeight;


	// ========================================================
	// Spawn
	// ========================================================

	FActorSpawnParameters Params;

	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


	ANoteActor* Note =
		GetWorld()->SpawnActor<ANoteActor>(
			Class,
			StartPos,
			GetActorRotation(),
			Params
		);


	if (!Note)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"NoteSpawner: Note生成失敗"
			)
		);

		return;
	}


	// ========================================================
	// Hold
	// ========================================================

	AHoldNoteActor* Hold =
		Cast<AHoldNoteActor>(Note);


	if (Hold)
	{
		Hold->SetHoldDuration(
			HoldDuration
		);
	}


	// ========================================================
	// Launch
	// ========================================================

	Note->Launch(
		StartPos,
		GetLaneTarget(LaneIndex),
		ArcHeight,
		TravelTime,
		LaneColors[LaneIndex],
		LaneIndex
	);


	// ========================================================
	// Judge Register
	// ========================================================

	if (Judge)
	{
		Judge->RegisterNote(Note);
	}


	// ========================================================
	// Lane Busy
	// ========================================================

	if (
		LaneBusyUntil.Num() <
		LaneColors.Num()
		)
	{
		LaneBusyUntil.SetNumZeroed(
			LaneColors.Num()
		);
	}


	if (Hold)
	{
		LaneBusyUntil[LaneIndex] =
			Hold->GetEndTime();
	}
	else
	{
		LaneBusyUntil[LaneIndex] =
			Note->GetHitTime();
	}
}


// ============================================================
// Get Lane Target
// ============================================================

FVector ANoteSpawner::GetLaneTarget(
	int32 LaneIndex
) const
{
	// ========================================================
	// LaneTargetOffsetsを使用
	// ========================================================

	if (
		LaneTargetOffsets.Num() ==
		LaneColors.Num()
		&&
		LaneTargetOffsets.IsValidIndex(
			LaneIndex
		)
		)
	{
		return
			GetActorLocation()
			+
			GetActorQuat().RotateVector(
				LaneTargetOffsets[LaneIndex]
			);
	}


	// ========================================================
	// Fallback
	// ========================================================

	const float Offset =
		(
			LaneIndex
			-
			(
				LaneColors.Num() - 1
				)
			*
			0.5f
			)
		*
		LaneSpacing;


	return
		GetActorLocation()
		+
		GetActorRightVector()
		*
		Offset;
}


// ============================================================
// Draw Judge Line
// ============================================================

void ANoteSpawner::DrawJudgeLine() const
{
	const int32 LaneNum =
		LaneColors.Num();


	if (LaneNum == 0)
	{
		return;
	}


	const UWorld* World =
		GetWorld();


	if (!World)
	{
		return;
	}


	// ========================================================
	// Lane connection
	// ========================================================

	for (
		int32 Lane = 0;
		Lane + 1 < LaneNum;
		++Lane
		)
	{
		DrawDebugLine(
			World,
			GetLaneTarget(Lane),
			GetLaneTarget(Lane + 1),
			FColor::White,
			false,
			-1.0f,
			0,
			2.0f
		);
	}


	// ========================================================
	// Note outline
	// ========================================================

	const ANoteActor* NoteDefaults =
		NoteClass
		?
		NoteClass->GetDefaultObject<ANoteActor>()
		:
		GetDefault<ANoteActor>();


	if (!NoteDefaults)
	{
		return;
	}


	TArray<FVector> Outline;

	NoteDefaults->GetPlateOutline(
		Outline
	);


	if (Outline.Num() < 2)
	{
		return;
	}


	const FQuat Rotation =
		GetActorQuat();


	for (
		int32 Lane = 0;
		Lane < LaneNum;
		++Lane
		)
	{
		const FVector Center =
			GetLaneTarget(Lane);


		const FColor Color =
			LaneColors[Lane]
			.ToFColor(true);


		for (
			int32 i = 0;
			i < Outline.Num();
			++i
			)
		{
			const FVector P0 =
				Center
				+
				Rotation.RotateVector(
					Outline[i]
				);


			const FVector P1 =
				Center
				+
				Rotation.RotateVector(
					Outline[
						(i + 1)
							%
							Outline.Num()
					]
				);


			DrawDebugLine(
				World,
				P0,
				P1,
				Color,
				false,
				-1.0f,
				0,
				3.0f
			);
		}
	}

}
// ============================================================
// Auto Spawn
// ============================================================

void ANoteSpawner::AutoSpawn()
{
	// Chartモードではランダム生成しない
	if (bUseChart)
	{
		return;
	}

	// ゲームオーバーなら停止
	if (Judge && Judge->IsGameOver())
	{
		GetWorldTimerManager().ClearTimer(AutoSpawnTimer);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("NoteSpawner: AutoSpawn停止 - GameOver")
		);

		return;
	}

	switch (SpawnPattern)
	{
	case ENoteSpawnPattern::AllLanes:
		SpawnWave();
		break;

	case ENoteSpawnPattern::RandomLane:
		SpawnRandomNote();
		break;

	default:
		break;
	}
}