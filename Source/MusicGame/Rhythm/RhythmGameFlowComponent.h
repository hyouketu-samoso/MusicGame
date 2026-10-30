// ゲーム画面からリザルト画面・リトライ画面へ移る係。NoteSpawner に付いている。
//
// ・曲の終わり : 譜面の時間が曲の長さを過ぎた、かつ譜面の判定がすべて終わった
//               → スコアの FinishSong → ResultDelay 秒後にリザルト画面（RhythmResultWidget）
// ・ゲームオーバー : スコアの OnGameOver → RetryDelay 秒後にリトライ画面（WBP_GameOver。BGM は画面が出るまで鳴らす）
//
// 曲の長さは「SongSoundOverride → 選択中の曲（RhythmMusicPlayer の SongData）の BGM の長さ
// → 最後のノーツ + EndMarginAfterLastNote 秒」の順に決める。
// リザルトに渡すデータ（スコア・判定数・最大コンボ・FAST/LATE・曲名・難易度）もここで集める。
// ランク・FULL COMBO / ALL PERFECT・NEW RECORD の決め方は未定のため、まだ入れていない。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RhythmTypes.h"
#include "RhythmResultWidget.h"
#include "RhythmGameFlowComponent.generated.h"

class ARhythmJudge;
class ANoteSpawner;
class ARhythmMusicPlayer;
class URhythmScoreComponent;
class USoundBase;
class USoundClass;
class UUserWidget;

// リトライ画面を出すとき（ゲームオーバー）。UI 担当が Blueprint で処理するときはここに繋ぐ
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRetryScreenRequested);

// リザルト画面を出すとき（曲の終わり）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResultScreenRequested, const FRhythmResultData&, ResultData);

UCLASS(ClassGroup = (Rhythm), meta = (BlueprintSpawnableComponent))
class URhythmGameFlowComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	URhythmGameFlowComponent();

	// 判定役の通知に登録する（NoteSpawner が判定役を見つけたあとに呼ぶ）
	void BindJudge(ARhythmJudge* InJudge);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// いまの結果からリザルト画面に渡すデータを作る
	UFUNCTION(BlueprintCallable, Category = "Rhythm Flow")
	FRhythmResultData BuildResultData() const;

	// 曲の長さ（秒、譜面の時間で比べる）
	UFUNCTION(BlueprintPure, Category = "Rhythm Flow")
	float GetSongLength() const;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm Flow")
	FOnRetryScreenRequested OnRetryScreenRequested;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm Flow")
	FOnResultScreenRequested OnResultScreenRequested;

protected:

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ---------- 画面 ----------

	// GAME CLEAR 表示ウィジェット
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Achievement")
	TSubclassOf<UUserWidget> GameClearWidgetClass;

	// ALL PERFECT 表示ウィジェット
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Achievement")
	TSubclassOf<UUserWidget> AllPerfectWidgetClass;

	// FULL COMBO 表示ウィジェット
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Achievement")
	TSubclassOf<UUserWidget> FullComboWidgetClass;

	// NEW RECORD 表示ウィジェット
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Achievement")
	TSubclassOf<UUserWidget> NewRecordWidgetClass;

	// 表示する実績（優先順位により1つだけ選択）
	UPROPERTY()
	TSubclassOf<UUserWidget> PendingAchievementClass;

	// リザルト画面（WBP_Result）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Widget")
	TSubclassOf<URhythmResultWidget> ResultWidgetClass;

	// リトライ（ゲームオーバー）画面。初期値は WBP_GameOver。空のときは OnRetryScreenRequested を出すだけ。
	// UGameOverWidget なら Open() を呼ぶ（画面に出す・ポーズ・BGM 停止・入力の切り替えは画面が行う）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Widget")
	TSubclassOf<UUserWidget> RetryWidgetClass;

	// リトライ画面を出している間、ゲームを止める（UGameOverWidget 以外の画面のときだけ。リザルト画面は自分で止める）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Widget")
	bool bPauseWhileRetryShown = true;

	// ---------- タイミング ----------

	// 曲が終わってからクリア表示を開始するまで（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Timing",
		meta = (ClampMin = 0.0, Units = "s"))
	float ResultDelay = 0.0f;

	// ゲームクリア画面を最低限表示する時間（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Achievement",
		meta = (ClampMin = "0.1", Units = "s"))
	float GameClearMinimumDisplayDuration = 4.0f;

	// 実績表示時間（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Achievement",
		meta = (ClampMin = "0.1"))
	float AchievementDuration = 4.0f;

	// ゲームオーバーからリトライ画面を出すまで（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Timing", meta = (ClampMin = 0.0, Units = "s"))
	float RetryDelay = 1.5f;

	// ゲームオーバーになった瞬間に BGM を止める（RhythmMusicPlayer::StopMusic）。
	// false なら、リトライ画面が出るまで鳴らしておく（WBP_GameOver は開くときに BGM を止める）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Timing")
	bool bStopMusicOnGameOver = false;

	// ゲームオーバー画面を開く瞬間に鳴らす SE（初期値は SE_system_failed）。
	// 画面を開くとゲームが一時停止するので、一時停止中でも鳴る UI 用の音として鳴らす
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Sound")
	TObjectPtr<USoundBase> GameOverSound = nullptr;

	// 上の SE に使う Sound Class（ポーズ画面の SE 音量スライダーが効くように SCL_SE）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Sound")
	TObjectPtr<USoundClass> GameOverSoundClass = nullptr;

	// ---------- 曲の長さ ----------

	// 曲の長さを取る BGM（手動で上書きしたいときだけ設定する）。空なら選択中の曲の BGM を使う
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Song")
	TObjectPtr<USoundBase> SongSoundOverride = nullptr;

	// BGM の長さが分からないとき、最後のノーツから何秒後を曲の終わりにするか
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Song", meta = (ClampMin = 0.0, Units = "s"))
	float EndMarginAfterLastNote = 2.0f;

	// ---------- 集計 ----------

	// FAST / LATE に Perfect も数える（false なら Great・Good・Bad だけ）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Flow|Result")
	bool bCountPerfectInFastLate = false;

private:

	UFUNCTION()
	void HandleNoteJudged(ERhythmJudgement Judgement, int32 LaneIndex, float TimingError, int32 Combo, ENoteJudgePoint Point);

	UFUNCTION()
	void HandleSongFinished(bool bCleared, double FinalScore);

	UFUNCTION()
	void HandleGameOver();

	UFUNCTION()
	void HandleGameClearNiagaraFinished();

	void ShowResultScreen();
	void ShowRetryScreen();

	// ゲームオーバー画面を開く瞬間の SE（一時停止中でも鳴る UI 用の音）
	void PlayGameOverSound();

	// クリア・実績表示
	void ShowGameClearThenAchievement();
	void ShowPendingAchievement();
	void ShowAchievementWidget(TSubclassOf<UUserWidget> WidgetClass);

	// ゲームクリア画面の最低表示時間が経過したとき
	void HandleGameClearMinimumDisplayTimeElapsed();

	// Niagara終了と最低表示時間の両方を満たしたか確認する
	void TryAdvanceAfterGameClear();

	// ゲームクリア演出の終了を確認したか
	bool bGameClearEffectFinished = false;

	// ゲームクリア画面の最低表示時間が経過したか
	bool bGameClearMinimumDisplayTimeElapsed = false;

	// 次の画面への移行をすでに開始したか
	bool bGameClearTransitionStarted = false;

	// 曲の終わりの条件（譜面の時間が曲の長さを過ぎた、かつ判定がすべて終わった）を満たしたか
	bool IsSongEnded() const;

	// 画面を出して、入力を画面に向ける
	void ShowWidget(UUserWidget* Widget);

	ANoteSpawner* GetSpawner() const;
	URhythmScoreComponent* GetScore() const;
	ARhythmMusicPlayer* FindMusicPlayer() const;

	UPROPERTY()
	TObjectPtr<ARhythmJudge> Judge;

	UPROPERTY()
	TObjectPtr<UUserWidget> ShownWidget;

	FTimerHandle ScreenTimer;

	// 判定より早かった / 遅かった回数
	int32 FastCount = 0;
	int32 LateCount = 0;

	// リザルト・リトライのどちらかを出すと決めたか（二重に出さない）
	bool bScreenRequested = false;
};