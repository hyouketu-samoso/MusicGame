// テスト用ノーツ発射台。
// レベルに置くと、一定間隔でノーツを飛ばす
// （全レーン同時 / ランダムなレーンに1個）。
//
// このActorの位置が「判定ラインの中心」。
// Actorの前方（X+）の奥からノーツが飛んでくる。
// プレイヤー（カメラ）はこのActorの後ろに立ち、前方を向いて見る想定。

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


// ============================================================
// Random Spawn Pattern
// ============================================================

UENUM(BlueprintType)
enum class ENoteSpawnPattern : uint8
{
    AllLanes
    UMETA(DisplayName = "All Lanes"),

    RandomLane
    UMETA(DisplayName = "Random Lane")
};


// ============================================================
// Note Spawner
// ============================================================

UCLASS()
class MUSICGAME_API ANoteSpawner : public AActor
{
    GENERATED_BODY()

public:

    ANoteSpawner();


protected:

    // ========================================================
    // Actor Lifecycle
    // ========================================================

    virtual void BeginPlay() override;

    virtual void OnConstruction(
        const FTransform& Transform
    ) override;


public:

    virtual void Tick(
        float DeltaTime
    ) override;


    // ========================================================
    // Camera
    // ========================================================

protected:

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


    // ========================================================
    // Note
    // ========================================================

    // 通常ノーツのクラス
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Note"
    )
    TSubclassOf<ANoteActor> NoteClass;


    // ホールドノーツのクラス
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Note"
    )
    TSubclassOf<AHoldNoteActor> HoldNoteClass;


    // RhythmJudge
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Note"
    )
    ARhythmJudge* Judge = nullptr;


    // ========================================================
    // Lane
    // ========================================================

    // レーンごとの色
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Lane"
    )
    TArray<FLinearColor> LaneColors;


    // 判定ラインにおける各レーンの位置
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Lane"
    )
    TArray<FVector> LaneTargetOffsets;


    // レーン間隔
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Lane",
        meta = (Units = "cm")
    )
    float LaneSpacing = 150.0f;


    // ========================================================
    // Movement
    // ========================================================

    // 判定ラインからどれだけ奥から生成するか
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Movement"
    )
    float SpawnDistance = 5000.0f;


    // ノーツ生成時の高さ
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Movement"
    )
    float SpawnHeight = 0.0f;


    // ノーツの軌道の高さ
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Movement"
    )
    float ArcHeight = 400.0f;


    // 生成位置から判定ラインまでの移動時間
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Movement"
    )
    float TravelTime = 2.5f;


    // ========================================================
    // Chart
    // ========================================================

    // JSONチャートを使用するか
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Chart"
    )
    bool bUseChart = true;


    // チャートファイル
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Chart"
    )
    FString ChartFilePath = TEXT("Chart/Demo.json");


    // ========================================================
    // Chart Start
    // ========================================================

    // 外部からチャートを開始する
    void StartChart();


    // ========================================================
    // Random Test
    // ========================================================

    // ランダム生成テストを使用するか
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Random"
    )
    bool bAutoSpawn = false;


    // ランダム生成間隔
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Random"
    )
    float SpawnInterval = 1.0f;


    // ホールドになる確率
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Random"
    )
    float HoldChance = 0.2f;


    // ランダムホールドの最小時間
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Random"
    )
    float HoldDurationMin = 1.0f;


    // ランダムホールドの最大時間
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Random"
    )
    float HoldDurationMax = 3.0f;


    // ========================================================
    // Debug
    // ========================================================

    // 判定ラインを表示する
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Debug"
    )
    bool bShowJudgeLine = true;


protected:

    // ========================================================
    // Camera Components
    // ========================================================

    UPROPERTY(
        VisibleAnywhere
    )
    UCameraComponent* Camera = nullptr;


#if WITH_EDITORONLY_DATA

    UPROPERTY()
    UArrowComponent* Arrow = nullptr;

#endif


    void UpdateCameraTransform();

    void ActivateAutoCamera();


    // ========================================================
    // Chart
    // ========================================================

    // JSONから読み込んだノーツ
    TArray<FNoteData> ChartNotes;


    // 次に生成するノーツのインデックス
    int32 NextChartNoteIndex = 0;


    // チャート読み込み済み
    bool bChartLoaded = false;


    // チャート実行中
    bool bChartRunning = false;


    // 曲開始後かどうか
    bool bChartPlaying = false;


    // 曲の基準となるWorldTime
    double ChartStartWorldTime = 0.0;


    // JSONチャートを読み込む
    void LoadChart();


    // 現在のSongTimeに応じてノーツを生成
    void UpdateChartSpawn(
        float CurrentSongTime
    );


    // ========================================================
    // Spawn
    // ========================================================

    // 実際のノーツ生成処理
    void SpawnNoteInternal(
        TSubclassOf<ANoteActor> Class,
        int32 LaneIndex,
        float HoldDuration
    );


    // 通常ノーツ
    void SpawnNote(
        int32 LaneIndex
    );


    // ホールドノーツ
    void SpawnHoldNote(
        int32 LaneIndex,
        float HoldDuration
    );


    // ランダムノーツ
    void SpawnRandomNote();


    // 全レーン生成
    void SpawnWave();


    // ランダム生成タイマーから呼ばれる
    void AutoSpawn();


    // レーンが使用可能か
    bool IsLaneFree(
        int32 LaneIndex
    ) const;


    // ========================================================
    // Lane
    // ========================================================

    // 指定レーンの判定ライン位置を取得
    FVector GetLaneTarget(
        int32 LaneIndex
    ) const;


    // ========================================================
    // Debug
    // ========================================================

    // 判定ラインを描画
    void DrawJudgeLine() const;


    // ========================================================
    // Random Spawn
    // ========================================================

    // ランダム生成用タイマー
    FTimerHandle AutoSpawnTimer;


    // 各レーンが次に使用可能になる時間
    TArray<double> LaneBusyUntil;


    // ランダム生成パターン
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Random"
    )
    ENoteSpawnPattern SpawnPattern =
        ENoteSpawnPattern::RandomLane;
};