#include "NoteSpawner.h"

#include "NoteActor.h"
#include "HoldNoteActor.h"
#include "RhythmJudge.h"
#include "ChartImporter.h"
#include "RhythmSettingsSubsystem.h"
#include "RhythmGameSettings.h"
#include "RhythmFeedbackComponent.h"
#include "Engine/GameInstance.h"

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
	// Feedback（判定エフェクト・SE）
	// ========================================================

	Feedback =
		CreateDefaultSubobject<URhythmFeedbackComponent>(
			TEXT("Feedback")
		);


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

		// 位置・向き・視野角は Camera の設定項目から決める
		UpdateCameraTransform();

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


	// ========================================================
	// 4 Lane Targets（V字）
	//
	// プレイ画面UI（WBP_GameMain）の判定バー4本に重なる位置。内側のレーンほど手前。
	// カメラが初期値（距離 700・高さ 400・角度 -15°・視野角 90°）のときに合わせた値なので、
	// カメラを変えたらこちらも合わせ直す
	// ========================================================

	LaneTargetOffsets =
	{
		FVector( -60.0f, -300.0f, 0.0f),
		FVector(-205.0f,  -90.0f, 0.0f),
		FVector(-205.0f,   90.0f, 0.0f),
		FVector( -60.0f,  300.0f, 0.0f)
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


	// 判定が出たらエフェクト・SE を出す
	if (Feedback && Judge)
	{
		Feedback->BindJudge(Judge);
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


	// 判定ラインの中心（このActorの位置）から、後ろへ CameraDistance・上へ CameraHeight の位置
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

	// ポーンの所持やカメラの自動切り替えで視点が戻されないようにする
	PC->bAutoManageActiveCameraTarget = false;

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


	// ========================================================
	// 難易度に合わせて譜面を選ぶ（HARD の譜面が空なら NORMAL を使う）
	// ========================================================

	FString SelectedChartPath = ChartFilePath;

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const URhythmSettingsSubsystem* Settings = GameInstance->GetSubsystem<URhythmSettingsSubsystem>())
		{
			if (Settings->GetDifficulty() == ERhythmDifficulty::Hard && !HardChartFilePath.IsEmpty())
			{
				SelectedChartPath = HardChartFilePath;
			}
		}
	}


	const FString FullPath =
		FPaths::Combine(
			FPaths::ProjectContentDir(),
			SelectedChartPath
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
			GetEffectiveTravelTime()
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
		GetEffectiveTravelTime()
	);


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"First SpawnTime = %.3f"
		),
		ChartNotes.Num() > 0
		? ChartNotes[0].Time - GetEffectiveTravelTime()
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
			GetEffectiveTravelTime();


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
			GetEffectiveTravelTime()
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
		GetEffectiveTravelTime(),
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
// Get Effective Travel Time
// ============================================================

float ANoteSpawner::GetEffectiveTravelTime() const
{
	// プレイヤーが選んだノーツ速度（1〜5）。テスト用の上書きがあればそちら
	int32 SpeedLevel = GetDefault<URhythmGameSettings>()->DefaultSpeedLevel;

	if (TestSpeedLevelOverride > 0)
	{
		SpeedLevel = TestSpeedLevelOverride;
	}
	else if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const URhythmSettingsSubsystem* Settings = GameInstance->GetSubsystem<URhythmSettingsSubsystem>())
		{
			SpeedLevel = Settings->GetNoteSpeedLevel();
		}
	}

	// 速度ごとの到達時間（プロジェクト設定 → ゲーム → Rhythm Game）
	return GetDefault<URhythmGameSettings>()->GetArrivalTime(SpeedLevel);
}


// ============================================================
// Get Lane Target
// ============================================================

FVector ANoteSpawner::GetLaneTarget(
	int32 LaneIndex
) const
{
	// ========================================================
	// レーンごとの判定位置が設定されていればそれを使う（初期値は UI に合わせた V字）
	// ========================================================

	if (LaneTargetOffsets.Num() == LaneColors.Num() &&
		LaneTargetOffsets.IsValidIndex(LaneIndex))
	{
		// スポナーの向きに合わせて回転
		return GetActorLocation() +
			GetActorQuat().RotateVector(
				LaneTargetOffsets[LaneIndex]
			);
	}


	// ========================================================
	// 無ければ中央揃えで左右に並べる（レーン 0 が左端）
	// ========================================================

	const float Offset =
		(LaneIndex - (LaneColors.Num() - 1) * 0.5f) * LaneSpacing;

	return GetActorLocation() +
		GetActorRightVector() * Offset;
}


// ============================================================
// Draw Judge Line
// ============================================================

void ANoteSpawner::DrawJudgeLine() const
{
	const int32 LaneNum = LaneColors.Num();
	if (LaneNum == 0)
	{
		return;
	}

	const UWorld* World = GetWorld();

	// となりのレーンの判定位置どうしを線でつなぐ（横一列でも V 字でも同じ）
	for (int32 Lane = 0; Lane + 1 < LaneNum; ++Lane)
	{
		DrawDebugLine(World, GetLaneTarget(Lane), GetLaneTarget(Lane + 1), FColor::White, false, -1.0f, 0, 2.0f);
	}

	// 飛ばすノーツの形（クラスの初期値）から輪郭を取り、各レーンの判定位置に描く。
	// ノーツがこの枠にぴったり重なった瞬間がパーフェクト
	const ANoteActor* NoteDefaults = NoteClass ? NoteClass->GetDefaultObject<ANoteActor>() : GetDefault<ANoteActor>();
	TArray<FVector> Outline;
	NoteDefaults->GetPlateOutline(Outline);

	const FQuat Rotation = GetActorQuat();
	for (int32 Lane = 0; Lane < LaneNum; ++Lane)
	{
		const FVector Center = GetLaneTarget(Lane);
		const FColor Color = LaneColors[Lane].ToFColor(true);
		for (int32 i = 0; i < Outline.Num(); ++i)
		{
			const FVector P0 = Center + Rotation.RotateVector(Outline[i]);
			const FVector P1 = Center + Rotation.RotateVector(Outline[(i + 1) % Outline.Num()]);
			DrawDebugLine(World, P0, P1, Color, false, -1.0f, 0, 3.0f);
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