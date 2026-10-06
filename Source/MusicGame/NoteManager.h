#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoteData.h"
#include "NoteManager.generated.h"

class ASoundNoteSpawner;

UCLASS()
class MUSICGAME_API ANoteManager : public AActor
{
	GENERATED_BODY()

public:

	ANoteManager();

	virtual void Tick(float DeltaTime) override;

	// 譜面データを設定
	void InitNotes(
		const TArray<FNoteData>& InNotes
	);

	// 譜面開始
	UFUNCTION(BlueprintCallable, Category = "Note Manager")
	void StartChart();

	// 譜面停止
	UFUNCTION(BlueprintCallable, Category = "Note Manager")
	void StopChart();

	// ノーツ生成処理
	void UpdateSpawn(
		float CurrentSongTime
	);

protected:

	virtual void BeginPlay() override;

	UPROPERTY(
		EditAnywhere,
		Category = "Chart"
	)
	TObjectPtr<ASoundNoteSpawner> NoteSpawner;

	UPROPERTY(
		EditAnywhere,
		Category = "Chart"
	)
	float StartDelay = 1.0f;

private:

	void SpawnNote(
		const FNoteData& Note
	);

	bool FindSpawner();

private:

	TArray<FNoteData> Notes;

	double ChartStartWorldTime = 0.0;

	bool bChartRunning = false;
};