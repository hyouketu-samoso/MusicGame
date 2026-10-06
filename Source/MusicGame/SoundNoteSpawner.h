#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoteData.h"
#include "SoundNoteSpawner.generated.h"

class ASoundNoteActor;
class UArrowComponent;

UCLASS()
class MUSICGAME_API ASoundNoteSpawner : public AActor
{
	GENERATED_BODY()

public:

	ASoundNoteSpawner();

	// 譜面データからノーツを生成
	ASoundNoteActor* SpawnNoteFromData(
		const FNoteData& NoteData
	);

	// テスト用
	UFUNCTION(BlueprintCallable, Category = "Note Spawner")
	void SpawnWave();

	// テスト用
	UFUNCTION(BlueprintCallable, Category = "Note Spawner")
	void SpawnNote(int32 LaneIndex);

	// ノーツの飛行時間
	float GetTravelTime() const
	{
		return TravelTime;
	}

protected:

	virtual void BeginPlay() override;

	// =========================
	// ノーツクラス
	// =========================

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Class"
	)
	TSubclassOf<ASoundNoteActor> NoteClass;

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Class"
	)
	TSubclassOf<ASoundNoteActor> TapNoteClass;

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Class"
	)
	TSubclassOf<ASoundNoteActor> FlickNoteClass;

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Class"
	)
	TSubclassOf<ASoundNoteActor> HoldNoteClass;

	// =========================
	// レーン
	// =========================

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Lane"
	)
	TArray<FLinearColor> LaneColors;

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Lane",
		meta = (Units = "cm")
	)
	float LaneSpacing = 150.0f;

	// =========================
	// ノーツ移動
	// =========================

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Movement",
		meta = (Units = "cm")
	)
	float SpawnDistance = 5000.0f;

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Movement",
		meta = (Units = "cm")
	)
	float SpawnHeight = 0.0f;

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Movement",
		meta = (Units = "cm")
	)
	float ArcHeight = 400.0f;

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Movement",
		meta = (
			ClampMin = "0.1",
			Units = "s"
			)
	)
	float TravelTime = 2.5f;

	// =========================
	// テスト用
	// =========================

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Test"
	)
	bool bAutoSpawn = false;

	UPROPERTY(
		EditAnywhere,
		Category = "Note Spawner|Test",
		meta = (
			ClampMin = "0.1",
			Units = "s",
			EditCondition = "bAutoSpawn"
			)
	)
	float SpawnInterval = 3.0f;

#if WITH_EDITORONLY_DATA

	UPROPERTY(
		VisibleAnywhere,
		Category = "Note Spawner"
	)
	TObjectPtr<UArrowComponent> Arrow;

#endif

private:

	FVector GetLaneTarget(
		int32 LaneIndex
	) const;

	TSubclassOf<ASoundNoteActor>
		GetNoteClassForType(
			const FString& Type
		) const;

	FTimerHandle AutoSpawnTimer;
};