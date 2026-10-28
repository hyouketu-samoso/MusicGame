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
#include "InputCoreTypes.h"
#include "ChartImporter.h"
#include "NoteSpawner.generated.h"

class ANoteActor;
class AHoldNoteActor;
class ARhythmJudge;
class URhythmFeedbackComponent;
class URhythmGameFlowComponent;
class URhythmHUDSyncComponent;
class UCameraComponent;
class UArrowComponent;
class UTexture2D;


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


    // 判定ライン（六角形・ガイド線）の表示・非表示を切り替える（JudgeLineToggleKey からも呼ばれる）
    UFUNCTION(BlueprintCallable, Category = "Note Spawner")
    void ToggleJudgeLine() { bShowJudgeLine = !bShowJudgeLine; }


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


    // 指定レーンの判定ライン位置を取得（ワールド座標。判定エフェクトの位置にも使う）
    UFUNCTION(BlueprintPure, Category = "Note Spawner")
    FVector GetLaneTarget(
        int32 LaneIndex
    ) const;


    // 指定レーンの判定地点でのノーツの向き（このActorの向きからのヨー回転、度）
    UFUNCTION(BlueprintPure, Category = "Note Spawner")
    float GetLaneTargetYaw(
        int32 LaneIndex
    ) const;


    // 指定レーンのノーツ・判定枠の大きさ（倍率）
    UFUNCTION(BlueprintPure, Category = "Note Spawner")
    float GetLaneNoteScale(
        int32 LaneIndex
    ) const;


    // 指定レーンのノーツ画像（無ければ nullptr = 六角形の板）
    UFUNCTION(BlueprintPure, Category = "Note Spawner")
    UTexture2D* GetLaneNoteTexture(
        int32 LaneIndex
    ) const;


    // 右側のレーンか（画像の六角形の向きが左右で逆）
    UFUNCTION(BlueprintPure, Category = "Note Spawner")
    bool IsRightLane(
        int32 LaneIndex
    ) const;


    // ========================================================
    // 譜面の進み具合（遷移の係 RhythmGameFlowComponent が曲の終わりを見るのに使う）
    // ========================================================

    // 譜面の時間（秒）。0 で最初のノーツ（譜面の 0 秒）が判定ラインに届く。譜面開始直後は負
    UFUNCTION(BlueprintPure, Category = "Note Spawner|Chart")
    float GetSongTime() const;

    // 譜面を進めている最中か（StartChart 後）
    UFUNCTION(BlueprintPure, Category = "Note Spawner|Chart")
    bool IsChartRunning() const { return bUseChart && bChartRunning; }

    // 譜面のノーツをすべて出し終えたか
    UFUNCTION(BlueprintPure, Category = "Note Spawner|Chart")
    bool HasSpawnedAllChartNotes() const { return bChartLoaded && NextChartNoteIndex >= ChartNotes.Num(); }

    // 最後のノーツが終わる譜面の時間（秒）。ホールドは終点の時間
    UFUNCTION(BlueprintPure, Category = "Note Spawner|Chart")
    float GetChartEndTime() const;

    // 譜面全体の判定数（通常ノーツ 1、ホールド 2（始点・終点））
    UFUNCTION(BlueprintPure, Category = "Note Spawner|Chart")
    int32 GetChartJudgeCount() const;

    // ゲーム開始からノーツ出現を開始するまでの待機時間
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Timing")
    float InitialNoteDelay = 0.1f;

    // 指定されたJSON譜面を読み込む
    bool LoadChartFromPath(const FString& RelativeChartPath);

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


    // 判定の演出（エフェクト・SE）。中身はこのコンポーネントの詳細パネルで設定する
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Note Spawner|Feedback"
    )
    URhythmFeedbackComponent* Feedback = nullptr;


    // ゲーム画面からリザルト・リトライ画面への遷移（曲の終わり・ゲームオーバーを見張る）
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Note Spawner|Flow"
    )
    URhythmGameFlowComponent* GameFlow = nullptr;


    // プレイ画面の UI（スコア・コンボ・体力・判定の文字）に実際の値を渡す
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Note Spawner|HUD"
    )
    URhythmHUDSyncComponent* HUDSync = nullptr;


    // ========================================================
    // Lane
    // ========================================================

    // レーンごとの色
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Lane"
    )
    TArray<FLinearColor> LaneColors;


    // 判定枠（判定ラインの六角形）の色（要素番号 = レーン番号）。
    // ノーツより鮮やかにしたいので別に持つ。要素が足りないレーンは LaneColors を使う
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Lane"
    )
    TArray<FLinearColor> JudgeLineColors;


    // 判定地点でのノーツ・判定枠の向き（要素番号 = レーン番号、ヨー回転・度）。
    // ノーツは出現時は横向きで、判定地点に近づくにつれてこの角度まで回る。
    // 初期値は外側2レーンを V字のガイド線（外側 → 内側の判定地点を結ぶ線）に沿わせた角度
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Lane"
    )
    TArray<float> LaneTargetYaws;


    // ノーツ・判定枠の大きさ（要素番号 = レーン番号、倍率）。
    // 外側2レーンは奥にあり斜めなので小さく見える。画面上で内側2レーンと同じ大きさに見えるよう大きくする
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Lane"
    )
    TArray<float> LaneNoteScales;


    // ノーツの画像（要素番号 = レーン番号）。設定したレーンは、六角形の板の代わりに画像を貼った板で表示する。
    // 判定枠も画像と同じ斜めの六角形で描く。空のレーンは今までどおり六角形の板
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Lane"
    )
    TArray<TObjectPtr<UTexture2D>> LaneNoteTextures;


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
    FString ChartFilePath = TEXT("Chart/AZURE_Easy.json");


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


    // プレイ中に判定ライン（六角形・ガイド線）の表示・非表示を切り替えるキー（押すたびに切り替わる）
    UPROPERTY(
        EditAnywhere,
        Category = "Note Spawner|Debug"
    )
    FKey JudgeLineToggleKey = EKeys::F2;


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
    //void LoadChart();


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