// テスト用ノーツ発射台。レベルに置くと、一定間隔で全レーンにノーツを飛ばす。
// このActorの位置が「判定ラインの中心」、Actorの前方（X+）の奥からノーツが飛んでくる。
// プレイヤー（カメラ）はこのActorの後ろに立ち、前方を向いて見る想定。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoteSpawner.generated.h"

class ANoteActor;
class UArrowComponent;

UCLASS()
class ANoteSpawner : public AActor
{
	GENERATED_BODY()

public:

	ANoteSpawner();

	// 全レーンに1個ずつノーツを飛ばす 
	UFUNCTION(BlueprintCallable, Category="Note Spawner")
	void SpawnWave();

	// 指定レーンにノーツを1個飛ばす 
	UFUNCTION(BlueprintCallable, Category="Note Spawner")
	void SpawnNote(int32 LaneIndex);

protected:

	virtual void BeginPlay() override;

	// 飛ばすノーツのクラス（BPで見た目を変えたい場合に差し替える）
	UPROPERTY(EditAnywhere, Category="Note Spawner")
	TSubclassOf<ANoteActor> NoteClass;

	// レーンごとの色。要素数 = レーン数 
	UPROPERTY(EditAnywhere, Category="Note Spawner")
	TArray<FLinearColor> LaneColors;

	// レーンの間隔（cm）
	UPROPERTY(EditAnywhere, Category="Note Spawner", meta=(Units="cm"))
	float LaneSpacing = 150.0f;

	// 判定ラインから出現地点までの距離（cm）
	UPROPERTY(EditAnywhere, Category="Note Spawner", meta=(Units="cm"))
	float SpawnDistance = 5000.0f;

	// 出現地点の高さ（判定ラインからの相対、cm） 
	UPROPERTY(EditAnywhere, Category="Note Spawner", meta=(Units="cm"))
	float SpawnHeight = 0.0f;

	// 山なりの高さ（cm） 
	UPROPERTY(EditAnywhere, Category="Note Spawner", meta=(Units="cm"))
	float ArcHeight = 400.0f;

	// 出現してから判定ラインに届くまでの時間 
	UPROPERTY(EditAnywhere, Category="Note Spawner", meta=(ClampMin=0.1, Units="s"))
	float TravelTime = 2.5f;

	// true なら BeginPlay から自動で SpawnWave を繰り返す 
	UPROPERTY(EditAnywhere, Category="Note Spawner|Test")
	bool bAutoSpawn = true;

	// 自動発射の間隔 
	UPROPERTY(EditAnywhere, Category="Note Spawner|Test", meta=(ClampMin=0.1, Units="s", EditCondition="bAutoSpawn"))
	float SpawnInterval = 3.0f;

#if WITH_EDITORONLY_DATA
	// エディタ上でノーツの飛んでくる向きを表示する矢印 
	UPROPERTY(VisibleAnywhere, Category="Note Spawner")
	TObjectPtr<UArrowComponent> Arrow;
#endif

private:

	// レーン番号から判定地点（ワールド座標）を求める 
	FVector GetLaneTarget(int32 LaneIndex) const;

	FTimerHandle AutoSpawnTimer;
};
