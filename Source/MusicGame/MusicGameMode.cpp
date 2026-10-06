#include "MusicGameMode.h"

#include "ChartImporter.h"
#include "NoteManager.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Misc/Paths.h"


AMusicGameMode::AMusicGameMode()
{
}

void AMusicGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogTemp,
		Error,
		TEXT("========== MusicGameMode BeginPlay ==========")
	);

	// ========================================
	// 譜面読み込み
	// ========================================

	UChartImporter* Importer =
		NewObject<UChartImporter>();

	if (!Importer)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("MusicGameMode: ChartImporter生成失敗")
		);

		return;
	}

	const bool bLoaded =
		Importer->LoadChart(
			FPaths::ProjectContentDir() /
			TEXT("Chart/chart.json"),
			Notes
		);

	UE_LOG(
		LogTemp,
		Error,
		TEXT("MusicGameMode: Chart Load=%s Notes=%d"),
		bLoaded ? TEXT("SUCCESS") : TEXT("FAILED"),
		Notes.Num()
	);

	// ========================================
	// NoteManager生成
	// ========================================

	NoteManager =
		GetWorld()->SpawnActor<ANoteManager>();

	if (!NoteManager)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("MusicGameMode: NoteManager生成失敗")
		);

		return;
	}

	NoteManager->InitNotes(Notes);

	// ========================================
	// 音楽開始
	// ========================================

	MusicAudioComponent =
		UGameplayStatics::SpawnSoundAttached(
			MusicSound,
			GetRootComponent()
		);

	StartTime =
		UGameplayStatics::GetTimeSeconds(
			GetWorld()
		);

	// ========================================
	// 譜面開始
	// ========================================

	UE_LOG(
		LogTemp,
		Error,
		TEXT("MusicGameMode: NoteManager->StartChart() を呼びます")
	);

	NoteManager->StartChart();

	// Tick有効化
	PrimaryActorTick.bCanEverTick = true;
}

void AMusicGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}