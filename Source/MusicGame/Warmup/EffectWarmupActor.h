// ============================================================
// EffectWarmupActor
//  役割 : ゲーム開始時に、使うエフェクトを一度ずつ出して、すぐ消す(ウォームアップ)
//         初めてエフェクトを出したときの「アセット読み込み・シェーダー準備」のかくつきを、
//         曲選択を表示している間に済ませておく
//  使い方 : レベルに1つ置いて、Warmup Systems に使うエフェクトを全部登録する
//           Spawn At Actor Location がオンなら、このアクターの位置にエフェクトを出す
//           (画面の範囲内で、動画の裏など、見えない場所に置く)
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EffectWarmupActor.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class MUSICGAME_API AEffectWarmupActor : public AActor
{
	GENERATED_BODY()

public:
	AEffectWarmupActor();

protected:
	virtual void BeginPlay() override;

	// ウォームアップするエフェクト(判定エフェクトなど、ゲーム中に出すものを全部入れる)
	UPROPERTY(EditAnywhere, Category = "Warmup")
	TArray<TSoftObjectPtr<UNiagaraSystem>> WarmupSystems;

	// 開始までの待ち(秒)。レベルが落ち着いてから始める
	UPROPERTY(EditAnywhere, Category = "Warmup", meta = (ClampMin = "0.0"))
	float StartDelay = 1.0f;

	// 出してから消すまでの時間(秒)。短すぎると、準備が終わらないことがある
	UPROPERTY(EditAnywhere, Category = "Warmup", meta = (ClampMin = "0.05"))
	float KeepSeconds = 0.5f;

	// 出すときの大きさ(小さくして、画面に目立たないようにする。0 にはしない)
	UPROPERTY(EditAnywhere, Category = "Warmup", meta = (ClampMin = "0.0001"))
	float WarmupScale = 0.01f;

	// true  : エフェクトを、このアクターの位置に出す(レベルで、画面の範囲内の見えない場所に置く)
	// false : エフェクトを、カメラの前に出す
	UPROPERTY(EditAnywhere, Category = "Warmup")
	bool bSpawnAtActorLocation = true;

	// カメラの前、どれくらい離れた場所に出すか(cm)(Spawn At Actor Location がオフのときだけ使う)
	UPROPERTY(EditAnywhere, Category = "Warmup", meta = (EditCondition = "!bSpawnAtActorLocation"))
	float DistanceFromCamera = 300.0f;

private:
	// 登録されたエフェクトを、裏で読み込む(読み込み中も、ゲームは止まらない)
	void RunWarmup();

	// 読み込みが終わったあと、1つずつ間をあけて出す(一度に出して重くならないように)
	void SpawnNext();

	// 出したものを、すべて消す
	void CleanupSpawned();

	TSharedPtr<struct FStreamableHandle> LoadHandle;
	int32 NextIndex = 0;
	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	TArray<TWeakObjectPtr<UNiagaraComponent>> Spawned;
};