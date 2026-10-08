#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChartImporter.h"
#include "NoteSpawner.generated.h"

class ANoteActor;
class AHoldNoteActor;
class ARhythmJudge;
class UCameraComponent;
class UArrowComponent;

UENUM(BlueprintType)
enum class ENoteSpawnPattern : uint8
{
	AllLanes	UMETA(DisplayName = "All Lanes"),
	RandomLane	UMETA(DisplayName = "Random Lane")
};

UCLASS()
class MUSICGAME_API ANoteSpawner : public AActor
{
	GENERATED_BODY()

public:

	ANoteSpawner();

protected:

	virtual void BeginPlay() override;

	virtual void OnConstruction(
		const FTransform& Transform
	) override;

public:

	virtual void Tick(
		float DeltaTime
	) override;


	// ============================================================
	// Camera
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera")
	bool bAutoCamera = true;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera")
	float CameraDistance = 1800.0f;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera")
	float CameraHeight = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera")
	float CameraPitch = -25.0f;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera")
	float CameraFieldOfView = 60.0f;


	// ============================================================
	// Note
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Note")
	TSubclassOf<ANoteActor> NoteClass;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Note")
	TSubclassOf<AHoldNoteActor> HoldNoteClass;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Note")
	ARhythmJudge* Judge = nullptr;


	// ============================================================
	// Lane
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Lane")
	TArray<FLinearColor> LaneColors;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Lane")
	TArray<FVector> LaneTargetOffsets;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Lane")
	float LaneSpacing = 150.0f;


	// ============================================================
	// Movement
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Movement")
	float SpawnDistance = 5000.0f;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Movement")
	float SpawnHeight = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Movement")
	float ArcHeight = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Movement")
	float TravelTime = 2.5f;


	// ============================================================
	// Chart
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Chart")
	bool bUseChart = true;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Chart")
	FString ChartFilePath =
		TEXT("Chart/Demo.json");


	// ============================================================
	// Chart Start
	// ============================================================

	// MusicGameModeなど外部から開始したい場合に使用
	void StartChart();


	// ============================================================
	// Random Test
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Random")
	bool bAutoSpawn = false;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Random")
	float SpawnInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Random")
	float HoldChance = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Random")
	float HoldDurationMin = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Random")
	float HoldDurationMax = 3.0f;


	// ============================================================
	// Debug
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Debug")
	bool bShowJudgeLine = true;


protected:

	// ============================================================
	// Camera
	// ============================================================

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera = nullptr;

#if WITH_EDITORONLY_DATA

	UPROPERTY()
	UArrowComponent* Arrow = nullptr;

#endif

	void UpdateCameraTransform();

	void ActivateAutoCamera();


	// ============================================================
	// Chart
	// ============================================================

	TArray<FNoteData> ChartNotes;

	int32 NextChartNoteIndex = 0;

	bool bChartLoaded = false;

	bool bChartRunning = false;

	bool bChartPlaying = false;


	// 曲が始まるWorldTime
	double ChartStartWorldTime = 0.0;


	void LoadChart();

	void UpdateChartSpawn(
		float CurrentSongTime
	);


	// ============================================================
	// Spawn
	// ============================================================

	void SpawnNoteInternal(
		TSubclassOf<ANoteActor> Class,
		int32 LaneIndex,
		float HoldDuration
	);

	void SpawnNote(
		int32 LaneIndex
	);

	void SpawnHoldNote(
		int32 LaneIndex,
		float HoldDuration
	);

	void SpawnRandomNote();

	void SpawnWave();

	void AutoSpawn();

	bool IsLaneFree(
		int32 LaneIndex
	) const;


	// ============================================================
	// Lane
	// ============================================================

	FVector GetLaneTarget(
		int32 LaneIndex
	) const;


	// ============================================================
	// Debug
	// ============================================================

	void DrawJudgeLine() const;


	// ============================================================
	// Random Spawn
	// ============================================================

	FTimerHandle AutoSpawnTimer;

	TArray<double> LaneBusyUntil;

	UPROPERTY(EditAnywhere, Category = "Note Spawner|Random")
	ENoteSpawnPattern SpawnPattern =
		ENoteSpawnPattern::RandomLane;
};