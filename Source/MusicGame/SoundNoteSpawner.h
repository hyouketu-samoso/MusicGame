#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoteData.h"
#include "SoundNoteSpawner.generated.h"

class ASoundNoteActor;
class AHoldNoteActor;
class ARhythmJudge;
class UArrowComponent;

UCLASS()
class MUSICGAME_API ASoundNoteSpawner : public AActor
{
	GENERATED_BODY()

public:

	ASoundNoteSpawner();

	virtual void BeginPlay() override;

	// 譜面からノーツを生成
	ASoundNoteActor* SpawnNoteFromData(
		const FNoteData& NoteData
	);

	// 自動生成テスト
	UFUNCTION()
	void SpawnWave();

	// ノーツ生成
	void SpawnNote(
		int32 LaneIndex
	);

	// ノーツが判定ラインまで到達する時間
	UFUNCTION(BlueprintPure, Category = "Note Spawner")
	float GetTravelTime() const
	{
		return TravelTime;
	}

protected:

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Note"
	)
	TSubclassOf<ASoundNoteActor> NoteClass;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Note"
	)
	TSubclassOf<ASoundNoteActor> TapNoteClass;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Note"
	)
	TSubclassOf<ASoundNoteActor> FlickNoteClass;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Note"
	)
	TSubclassOf<ASoundNoteActor> HoldNoteClass;

	// 判定役
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Rhythm"
	)
	TObjectPtr<ARhythmJudge> RhythmJudge;

	// 4レーン分の色
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Lane"
	)
	TArray<FLinearColor> LaneColors;

	// レーン間隔
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Lane"
	)
	float LaneSpacing = 200.0f;

	// ノーツの出現位置までの距離
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Note"
	)
	float SpawnDistance = 2000.0f;

	// ノーツの高さ
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Note"
	)
	float SpawnHeight = 0.0f;

	// 判定ラインまでの移動時間
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Note"
	)
	float TravelTime = 2.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Note"
	)
	float ArcHeight = 0.0f;

	// テスト用自動生成
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Test"
	)
	bool bAutoSpawn = false;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Test"
	)
	float SpawnInterval = 1.0f;

private:

	TSubclassOf<ASoundNoteActor>
		GetNoteClassForType(
			const FString& Type
		) const;

	FVector GetLaneTarget(
		int32 LaneIndex
	) const;

	FTimerHandle AutoSpawnTimer;
};