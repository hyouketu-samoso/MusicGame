#include "MusicGameMode.h"

#include "NoteSpawner.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"


AMusicGameMode::AMusicGameMode()
{
}


void AMusicGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("========== MusicGameMode BeginPlay ==========")
	);


	// ============================================================
	// NoteSpawner取得
	// ============================================================

	ANoteSpawner* NoteSpawner =
		Cast<ANoteSpawner>(
			UGameplayStatics::GetActorOfClass(
				GetWorld(),
				ANoteSpawner::StaticClass()
			)
		);


	if (!NoteSpawner)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("MusicGameMode: ANoteSpawner not found.")
		);

		return;
	}


	UE_LOG(
		LogTemp,
		Warning,
		TEXT("MusicGameMode: ANoteSpawner found")
	);


	// ============================================================
	// BGM
	// ============================================================
	//
	// 現段階ではNoteSpawnerが自動でStartChartするため、
	// BGMはここで開始する。
	//
	// 後でChartStartWorldTimeと完全同期させる。
	// ============================================================

	if (MusicSound)
	{
		MusicAudioComponent =
			UGameplayStatics::SpawnSoundAttached(
				MusicSound,
				GetRootComponent()
			);

		if (MusicAudioComponent)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("MusicGameMode: BGM Start")
			);
		}
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("MusicGameMode: MusicSound has not been assigned.")
		);
	}


	// ============================================================
	// StartTime
	// ============================================================

	StartTime =
		UGameplayStatics::GetTimeSeconds(
			GetWorld()
		);


	// ============================================================
	// Tick
	// ============================================================

	PrimaryActorTick.bCanEverTick = true;
}


void AMusicGameMode::Tick(
	float DeltaSeconds
)
{
	Super::Tick(DeltaSeconds);
}