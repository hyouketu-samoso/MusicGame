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


    // 判定ラインをエディタのビューポートでも表示するため、プレイ中以外もTickさせる
    virtual bool ShouldTickIfViewportsOnly() const override
    {
        return bShowJudgeLine;
    }


    // ========================================================
    // Blueprint から呼べる操作
    // ========================================================

    // 譜面を開始する（譜面が読み込まれている必要がある）
    UFUNCTION(BlueprintCallable, Category = "Note Spawner")
    void StartChart();


    // 全レーンに1個ずつノーツを飛ばす
    UFUNCTION(BlueprintCallable, Category = "Note Spawner")
    void SpawnWave();


    // 指定レーンにノーツを1個飛ばす
    UFUNCTION(BlueprintCallable, Category = "Note Spawner")
    void SpawnNote(
        int32 LaneIndex
    );


    // 指定レーンにホールドノーツを1個飛ばす
    UFUNCTION(BlueprintCallable, Category = "Note Spawner")
    void SpawnHoldNote(
        int32 LaneIndex,
        float HoldDuration
    );


    // ランダムなレーンにノーツを1個飛ばす（HoldChance の確率でホールド）
    UFUNCTION(BlueprintCallable, Category = "Note Spawner")
    void SpawnRandomNote();


    // ========================================================
    // Camera
    // ========================================================

protected:

    // true なら Play 開始時にこのActorのカメラの視点に切り替える
    UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera")
    bool bAutoCamera = true;

    // 判定ラインからカメラまでの距離（後ろ方向）
    // ※ LaneTargetOffsets の V字はカメラが初期値のときに UI に合わせた値。変えたら合わせ直す
    UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera", meta = (Units = "cm", EditCondition = "bAutoCamera"))
    float CameraDistance = 700.0f;

    // 判定ラインからのカメラの高さ
    UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera", meta = (Units = "cm", EditCondition = "bAutoCamera"))
    float CameraHeight = 400.0f;

    // カメラの上下の向き。マイナスで見下ろす
    UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera", meta = (ClampMin = -89.0, ClampMax = 89.0, Units = "deg", EditCondition = "bAutoCamera"))
    float CameraPitch = -15.0f;

    // 視野角（左右）。小さいほどズーム
    UPROPERTY(EditAnywhere, Category = "Note Spawner|Camera", meta = (ClampMin = 10.0, ClampMax = 170.0, Units = "deg", EditCondition = "bAutoCamera"))
    float CameraFieldOfView = 90.0f;


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


    // 判定ラインにおける各レーンの位置（このActorからの相対位置、cm）
    // X = 前後（マイナスで手前）、Y = 左右（マイナスで左）、Z = 上下。
    // 初期値はプレイ画面UI（WBP_GameMain）の判定バー4本に重なる V字（カメラ初期値のとき）。
    // 要素数がレーン数と同じときに使い、空にすると LaneSpacing 間隔の横一列になる
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


    // ※ 生成位置から判定ラインまでの移動時間（到達時間）は、プレイヤーが選んだノーツ速度（1〜5）で決まる。
    //   速度ごとの秒数は「プロジェクト設定 → ゲーム → Rhythm Game」で管理（難易度調整仕様書の調整値一覧）


    // テスト用：1〜5 にすると、プレイヤー設定の代わりにこのノーツ速度を使う（0 = プレイヤー設定。セーブデータは変えない）
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Movement",
        meta = (ClampMin = 0, ClampMax = 5)
    )
    int32 TestSpeedLevelOverride = 0;


    // ノーツ速度を反映した移動時間（到達時間）
    UFUNCTION(BlueprintPure, Category = "Note Spawner")
    float GetEffectiveTravelTime() const;


    // ========================================================
    // Chart
    // ========================================================

    // JSONチャートを使用するか
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Chart"
    )
    bool bUseChart = true;


    // チャートファイル（NORMAL）。Content フォルダからの相対パス
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Chart"
    )
    FString ChartFilePath = TEXT("Chart/Demo.json");


    // チャートファイル（HARD）。空なら NORMAL と同じ譜面を使う
    // 難易度で変わるのは譜面だけ（速度・判定幅・スコアは共通）
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Chart"
    )
    FString HardChartFilePath;


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

    // ※ SpawnNote / SpawnHoldNote / SpawnRandomNote / SpawnWave は public（Blueprint から呼べる）


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