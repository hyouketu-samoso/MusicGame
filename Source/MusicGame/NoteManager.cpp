#include "NoteManager.h"

#include "SoundNoteSpawner.h"
#include "SoundNoteActor.h"

#include "Engine/World.h"
#include "EngineUtils.h"


ANoteManager::ANoteManager()
{
	PrimaryActorTick.bCanEverTick = true;
}


// ============================================================
// BeginPlay
// ============================================================

void ANoteManager::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogTemp,
		Error,
		TEXT("！！！！新しい NoteManager.cpp が実行されています！！！！")
	);

	if (!NoteSpawner)
	{
		FindSpawner();
	}
}

// ============================================================
// Tick
// ============================================================

void ANoteManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bChartRunning)
	{
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	const double CurrentWorldTime =
		GetWorld()->GetTimeSeconds();

	const float CurrentSongTime =
		static_cast<float>(
			CurrentWorldTime -
			ChartStartWorldTime
			);

	UpdateSpawn(CurrentSongTime);
}


// ============================================================
// 譜面設定
// ============================================================

void ANoteManager::InitNotes(
	const TArray<FNoteData>& InNotes
)
{
	Notes.Empty();

	Notes = InNotes;

	Notes.Sort(
		[](const FNoteData& A, const FNoteData& B)
		{
			return A.Time < B.Time;
		}
	);

	for (FNoteData& Note : Notes)
	{
		Note.bSpawned = false;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"NoteManager: Initialized %d notes."
		),
		Notes.Num()
	);

	for (const FNoteData& Note : Notes)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"Note: Time=%.3f Duration=%.3f Lane=%d Type=%s"
			),
			Note.Time,
			Note.Duration,
			Note.Lane,
			*Note.Type
		);
	}
}


// ============================================================
// 譜面開始
// ============================================================

void ANoteManager::StartChart()
{
	UE_LOG(
		LogTemp,
		Error,
		TEXT(
			"========== StartChart() call =========="
		)
	);

	if (!GetWorld())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("NoteManager: StartChart - GetWorld() is null"));
		return;
	}

	if (Notes.Num() == 0)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"NoteManager: StartChart - ノーツが0個です"
			)
		);

		return;
	}

	for (FNoteData& Note : Notes)
	{
		Note.bSpawned = false;
	}

	ChartStartWorldTime =
		GetWorld()->GetTimeSeconds() +
		StartDelay;

	bChartRunning = true;

	UE_LOG(
		LogTemp,
		Error,
		TEXT(
			"NoteManager: Chart started! Notes=%d StartTime=%.3f"
		),
		Notes.Num(),
		ChartStartWorldTime
	);
}


// ============================================================
// 譜面停止
// ============================================================

void ANoteManager::StopChart()
{
	bChartRunning = false;

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"NoteManager: Chart stopped."
		)
	);
}


// ============================================================
// ノーツ生成タイミング
// ============================================================

void ANoteManager::UpdateSpawn(float CurrentSongTime)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"NoteManager: UpdateSpawn CurrentSongTime=%.3f"
		),
		CurrentSongTime
	);

	if (!NoteSpawner)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"NoteManager: UpdateSpawn - NoteSpawner is null"
			)
		);

		return;
	}

	const float TravelTime =
		NoteSpawner->GetTravelTime();

	for (FNoteData& Note : Notes)
	{
		if (Note.bSpawned)
		{
			continue;
		}

		const float SpawnTime =
			Note.Time -
			TravelTime;

		if (CurrentSongTime >= SpawnTime)
		{
			SpawnNote(Note);

			Note.bSpawned = true;
		}
	}
}


// ============================================================
// ノーツ生成
// ============================================================

void ANoteManager::SpawnNote(
	const FNoteData& Note
)
{
	if (!NoteSpawner)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Error,
		TEXT(
			"NoteManager: SpawnNote Time=%.3f Lane=%d Type=%s"
		),
		Note.Time,
		Note.Lane,
		*Note.Type
	);

	ASoundNoteActor* SpawnedNote =
		NoteSpawner->SpawnNoteFromData(Note);

	if (!SpawnedNote)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"NoteManager: Failed to spawn "
				"Time=%.3f Lane=%d Type=%s"
			),
			Note.Time,
			Note.Lane,
			*Note.Type
		);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"NoteManager: Spawn SUCCESS"
			)
		);
	}
}


// ============================================================
// Spawner検索
// ============================================================

bool ANoteManager::FindSpawner()
{
	if (!GetWorld())
	{
		return false;
	}

	for (
		TActorIterator<ASoundNoteSpawner> It(GetWorld());
		It;
		++It
		)
	{
		NoteSpawner = *It;

		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"NoteManager: Found SoundNoteSpawner."
			)
		);

		return true;
	}

	UE_LOG(
		LogTemp,
		Error,
		TEXT(
			"NoteManager: SoundNoteSpawner not found."
		)
	);

	return false;
}