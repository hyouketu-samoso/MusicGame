#include "NoteSpawner.h"

#include "NoteActor.h"
#include "HoldNoteActor.h"
#include "RhythmJudge.h"
#include "ChartImporter.h"

#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
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

	// ========================================================
	// Root
	// ========================================================

	RootComponent =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("Root")
		);


#if WITH_EDITORONLY_DATA

	// ========================================================
	// Editor Arrow
	// ========================================================

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

	if (Camera)
	{
		Camera->SetupAttachment(RootComponent);

		// 今まで使用していたカメラ設定
		Camera->SetRelativeLocationAndRotation(
			FVector(
				-1800.0f,
				0.0f,
				1000.0f
			),
			FRotator(
				-25.0f,
				0.0f,
				0.0f
			)
		);

		Camera->SetFieldOfView(60.0f);

		Camera->bAutoActivate = true;
	}


	// ========================================================
	// Note Class
	// ========================================================

	NoteClass =
		ANoteActor::StaticClass();

	HoldNoteClass =
		AHoldNoteActor::StaticClass();


	// ========================================================
	// 4 Lane Colors
	// ========================================================

	LaneColors =
	{
		FLinearColor(0.0f, 0.9f, 1.0f),
		FLinearColor(0.1f, 0.3f, 1.0f),
		FLinearColor(1.0f, 0.2f, 0.8f),
		FLinearColor(1.0f, 0.85f, 0.1f)
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
	// RhythmJudge取得
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
		// Chart使用時はランダム生成を完全にOFF
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
// Tick
// ============================================================

void ANoteSpawner::Tick(
	float DeltaTime
)
{
	Super::Tick(DeltaTime);


	// ========================================================
	// Judge Line Debug
	// ========================================================

	if (bShowJudgeLine)
	{
		DrawJudgeLine();
	}


	// ========================================================
	// Chart
	// ========================================================

	if (!bUseChart ||
		!bChartRunning)
	{
		return;
	}

	if (!GetWorld())
	{
		return;
	}


	const double Now =
		GetWorld()->GetTimeSeconds();


	// --------------------------------------------------------
	// SongTime
	//
	// ChartStartWorldTime は
	// 「最初のノーツが判定ラインに到達する時刻」
	// ではなく、
	// 「SongTime = 0 になるWorldTime」
	//
	// StartChart() で TravelTime 分だけ未来に設定する。
	// --------------------------------------------------------

	const float CurrentSongTime =
		static_cast<float>(
			Now - ChartStartWorldTime
			);


	// ========================================================
	// Song Start
	// ========================================================

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


	// ========================================================
	// Chart Spawn
	// ========================================================

	UpdateChartSpawn(
		CurrentSongTime
	);
}


// ============================================================
// Update Camera Transform
// ============================================================

void ANoteSpawner::UpdateCameraTransform()
{
	if (!Camera)
	{
		return;
	}


	Camera->SetRelativeLocationAndRotation(
		FVector(
			-1800.0f,
			0.0f,
			1000.0f
		),
		FRotator(
			-25.0f,
			0.0f,
			0.0f
		)
	);

	Camera->SetFieldOfView(
		60.0f
	);
}


// ============================================================
// Activate Auto Camera
// ============================================================

void ANoteSpawner::ActivateAutoCamera()
{
	if (!GetWorld())
	{
		return;
	}

	if (!Camera)
	{
		return;
	}


	APlayerController* PC =
		GetWorld()->GetFirstPlayerController();

	if (!PC)
	{
		return;
	}


	Camera->SetActive(true);

	PC->SetViewTarget(this);


	UE_LOG(
		LogTemp,
		Warning,
		TEXT("NoteSpawner: Auto Camera Activated")
	);
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
		TEXT(
			"NoteSpawner: Chart loading: %s"
		),
		*FullPath
	);


	UChartImporter* Importer =
		NewObject<UChartImporter>();

	if (!Importer)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"NoteSpawner: ChartImporter生成失敗"
			)
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
			TEXT(
				"NoteSpawner: Chart読み込み失敗: %s"
			),
			*FullPath
		);

		return;
	}


	// ========================================================
	// Time順にソート
	// ========================================================

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
			"NoteSpawner: Chart読み込み成功 Notes=%d"
		),
		ChartNotes.Num()
	);


	// ========================================================
	// Chart Debug
	// ========================================================

	for (int32 i = 0;
		i < ChartNotes.Num();
		i++)
	{
		const FNoteData& Note =
			ChartNotes[i];


		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"Chart[%d] "
				"Time=%.3f "
				"Lane=%d "
				"Type=%s "
				"Duration=%.3f"
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


	// ========================================================
	// 重要
	//
	// 最初のノーツが
	//
	// Note.Time = 0
	//
	// の場合、
	//
	// SpawnTime = 0 - TravelTime
	//
	// になる。
	//
	// そのため SongTime = -TravelTime の時点で
	// 最初のノーツを生成する。
	//
	// TravelTime 秒後に
	// SongTime = 0
	// となり、ノーツが判定ラインへ到着する。
	// ========================================================

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
		TEXT(
			"========== CHART START =========="
		)
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


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"First SpawnTime = %.3f"
		),
		ChartNotes.Num() > 0
		? ChartNotes[0].Time - TravelTime
		: 0.0f
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


		// ====================================================
		// ノーツ生成時刻
		//
		// 例:
		//
		// Note.Time    = 0.0
		// TravelTime   = 2.5
		//
		// SpawnTime    = -2.5
		//
		// ====================================================

		const float SpawnTime =
			NoteData.Time -
			TravelTime;


		// まだ生成時刻ではない
		if (CurrentSongTime < SpawnTime)
		{
			break;
		}


		// ====================================================
		// Lane
		// ====================================================

		const int32 LaneCount =
			LaneColors.Num();


		if (LaneCount <= 0)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT(
					"NoteSpawner: LaneColorsが空です"
				)
			);

			return;
		}


		const int32 Lane =
			FMath::Clamp(
				NoteData.Lane,
				0,
				LaneCount - 1
			);


		// ====================================================
		// Note Class
		// ====================================================

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


		// ====================================================
		// Debug
		// ====================================================

		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"CHART SPAWN "
				"Index=%d "
				"SongTime=%.3f "
				"NoteTime=%.3f "
				"SpawnTime=%.3f "
				"Lane=%d "
				"Type=%s"
			),
			NextChartNoteIndex,
			CurrentSongTime,
			NoteData.Time,
			SpawnTime,
			Lane,
			*NoteData.Type
		);


		// ====================================================
		// Spawn
		// ====================================================

		SpawnNoteInternal(
			SpawnClass,
			Lane,
			HoldDuration
		);


		// ====================================================
		// 次のノーツ
		// ====================================================

		NextChartNoteIndex =
			NextChartNoteIndex + 1;
	}
}


// ============================================================
// Spawn Random Note
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
		Lane++
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
		const float Duration =
			FMath::FRandRange(
				HoldDurationMin,
				FMath::Max(
					HoldDurationMin,
					HoldDurationMax
				)
			);


		SpawnHoldNote(
			Lane,
			Duration
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
		Lane++
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
// Spawn Hold Note
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
		static_cast<double>(
			TravelTime
			);


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


	// ========================================================
	// Class Check
	// ========================================================

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


	// ========================================================
	// Lane Check
	// ========================================================

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
	// Spawn Position
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
	// Spawn Actor
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
	// Hold設定
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
	// 4レーンの判定位置
	//
	// 以前使っていたV字配置をそのまま使用
	// ========================================================

	static const FVector LaneOffsets[4] =
	{
		FVector(-60.0f, -300.0f, 0.0f),
		FVector(-205.0f, -90.0f, 0.0f),
		FVector(-205.0f, 90.0f, 0.0f),
		FVector(-60.0f, 300.0f, 0.0f)
	};


	const int32 SafeLane =
		FMath::Clamp(
			LaneIndex,
			0,
			3
		);


	// Actorのローカル座標をワールド座標へ変換
	return GetActorTransform().TransformPosition(
		LaneOffsets[SafeLane]
	);
}


// ============================================================
// Draw Judge Line
// ============================================================

void ANoteSpawner::DrawJudgeLine() const
{
	const int32 LaneNum =
		LaneColors.Num();


	if (LaneNum <= 0)
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
	// 各レーンの判定位置
	// ========================================================

	for (
		int32 Lane = 0;
		Lane < LaneNum;
		Lane++
		)
	{
		const FVector Center =
			GetLaneTarget(Lane);


		const FColor Color =
			LaneColors[Lane].ToFColor(true);


		// 判定位置を見やすくする
		DrawDebugSphere(
			World,
			Center,
			35.0f,
			16,
			Color,
			false,
			-1.0f,
			0,
			3.0f
		);


		// レーン番号位置の小さな縦線
		DrawDebugLine(
			World,
			Center - FVector(0.0f, 0.0f, 40.0f),
			Center + FVector(0.0f, 0.0f, 40.0f),
			Color,
			false,
			-1.0f,
			0,
			3.0f
		);
	}


	// ========================================================
	// V字のレーンライン
	// ========================================================

	for (
		int32 Lane = 0;
		Lane + 1 < LaneNum;
		Lane++
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


	// ========================================================
	// Game Over
	// ========================================================

	if (
		Judge &&
		Judge->IsGameOver()
		)
	{
		GetWorldTimerManager().ClearTimer(
			AutoSpawnTimer
		);


		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"NoteSpawner: AutoSpawn停止 - GameOver"
			)
		);


		return;
	}


	// ========================================================
	// Spawn Pattern
	// ========================================================

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