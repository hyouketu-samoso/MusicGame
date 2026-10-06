// テスト用ノーツ発射台。レベルに置くと、一定間隔でノーツを飛ばす（全レーン同時 / ランダムなレーンに1個）。
// このActorの位置が「判定ラインの中心」、Actorの前方（X+）の奥からノーツが飛んでくる。
// プレイヤー（カメラ）はこのActorの後ろに立ち、前方を向いて見る想定。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoteSpawner.generated.h"

class ANoteActor;
class AHoldNoteActor;
class ARhythmJudge;
class UArrowComponent;

// 自動発射の出し方
UENUM(BlueprintType)
enum class ENoteSpawnPattern : uint8
{
	AllLanes    UMETA(DisplayName="全レーン同時"),
	RandomLane  UMETA(DisplayName="ランダムなレーンに1個"),
};

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

	// 指定レーンにホールドノーツを1個飛ばす
	UFUNCTION(BlueprintCallable, Category="Note Spawner")
	void SpawnHoldNote(int32 LaneIndex, float HoldDuration);

	// ランダムなレーンにノーツを1個飛ばす（HoldChance の確率でホールドになる）
	UFUNCTION(BlueprintCallable, Category="Note Spawner")
	void SpawnRandomNote();

	virtual void Tick(float DeltaTime) override;

	// 判定ラインをエディタのビューポートでも表示するため、プレイ中以外もTickさせる
	virtual bool ShouldTickIfViewportsOnly() const override { return bShowJudgeLine; }

protected:

	virtual void BeginPlay() override;

	// 飛ばすノーツのクラス（BPで見た目を変えたい場合に差し替える）
	UPROPERTY(EditAnywhere, Category="Note Spawner")
	TSubclassOf<ANoteActor> NoteClass;

	// 飛ばすホールドノーツのクラス
	UPROPERTY(EditAnywhere, Category="Note Spawner")
	TSubclassOf<AHoldNoteActor> HoldNoteClass;

	// 飛ばしたノーツを判定する判定役。空ならレベル内から探し、無ければ自動で作る
	UPROPERTY(EditAnywhere, Category="Note Spawner")
	TObjectPtr<ARhythmJudge> Judge;

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

	// true なら BeginPlay から自動でノーツを飛ばし続ける
	UPROPERTY(EditAnywhere, Category="Note Spawner|Test")
	bool bAutoSpawn = true;

	// 自動発射の出し方
	UPROPERTY(EditAnywhere, Category="Note Spawner|Test", meta=(EditCondition="bAutoSpawn"))
	ENoteSpawnPattern SpawnPattern = ENoteSpawnPattern::RandomLane;

	// 自動発射の間隔
	UPROPERTY(EditAnywhere, Category="Note Spawner|Test", meta=(ClampMin=0.1, Units="s", EditCondition="bAutoSpawn"))
	float SpawnInterval = 1.0f;

	// ランダムなレーンに飛ばすとき、ホールドノーツになる確率（0 = 全部通常、1 = 全部ホールド）
	UPROPERTY(EditAnywhere, Category="Note Spawner|Test", meta=(ClampMin=0.0, ClampMax=1.0, EditCondition="bAutoSpawn"))
	float HoldChance = 0.3f;

	// ホールドの長さの範囲（この間でランダム）
	UPROPERTY(EditAnywhere, Category="Note Spawner|Test", meta=(ClampMin=0.1, Units="s", EditCondition="bAutoSpawn"))
	float HoldDurationMin = 0.5f;

	UPROPERTY(EditAnywhere, Category="Note Spawner|Test", meta=(ClampMin=0.1, Units="s", EditCondition="bAutoSpawn"))
	float HoldDurationMax = 1.5f;

	// 同じレーンで、前のノーツ（ホールドなら終点）から次のノーツまで最低限あける時間
	UPROPERTY(EditAnywhere, Category="Note Spawner|Test", meta=(ClampMin=0.0, Units="s", EditCondition="bAutoSpawn"))
	float MinLaneGap = 0.3f;

	// パーフェクトの位置（ノーツが判定ラインに届く位置）を線と円で表示する（仮表示）
	UPROPERTY(EditAnywhere, Category="Note Spawner|Debug")
	bool bShowJudgeLine = true;

	// 判定位置に表示する円の半径（ノーツの半径は 25cm）
	UPROPERTY(EditAnywhere, Category="Note Spawner|Debug", meta=(ClampMin=1.0, Units="cm", EditCondition="bShowJudgeLine"))
	float JudgeCircleRadius = 35.0f;

#if WITH_EDITORONLY_DATA
	// エディタ上でノーツの飛んでくる向きを表示する矢印 
	UPROPERTY(VisibleAnywhere, Category="Note Spawner")
	TObjectPtr<UArrowComponent> Arrow;
#endif

private:

	// レーン番号から判定地点（ワールド座標）を求める 
	FVector GetLaneTarget(int32 LaneIndex) const;

	// 自動発射のタイマーから呼ばれる。SpawnPattern に合わせて飛ばす
	void AutoSpawn();

	// 判定ラインを描画する
	void DrawJudgeLine() const;

	// ノーツを生成して飛ばす（HoldDuration > 0 ならホールド）
	void SpawnNoteInternal(TSubclassOf<ANoteActor> Class, int32 LaneIndex, float HoldDuration);

	// 自動発射で、このレーンに今ノーツを出しても前のノーツと重ならないか
	bool IsLaneFree(int32 LaneIndex) const;

	FTimerHandle AutoSpawnTimer;

	// レーンごとの「最後のノーツ（ホールドなら終点）が判定ラインに届く時刻」
	TArray<double> LaneBusyUntil;
};
