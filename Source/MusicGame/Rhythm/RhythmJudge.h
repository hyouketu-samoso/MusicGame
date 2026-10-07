// 判定役のActor。レーンの入力を受け取り、飛んでいるノーツとのタイミングのズレから判定を決める。
// レーンは4本（0=左外側 / 1=左内側 / 2=右内側 / 3=右外側）。
// プレイヤーコントローラーに入力コンポーネント（URhythmInputComponent）があれば、押した・離したはそちらから受け取る。
// 入力コンポーネントの「ホールド」通知（一定時間押し続けた）は入力の状態なので、判定には使わない。
// スコア・コンボ・体力は ScoreComponent（URhythmScoreComponent）で管理する。
// ホールドノーツは「始点を押したとき」と「終点で離したとき」の2回判定する。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "RhythmTypes.h"
#include "Input/RhythmInputTypes.h"
#include "RhythmJudge.generated.h"

class ANoteActor;
class AHoldNoteActor;
class URhythmScoreComponent;

// 判定が出たときの通知（UIや演出はこれに登録する）
// TimingError : 判定ラインに届く時刻とのズレ（秒）。マイナス = 早い、プラス = 遅い。ミスのときは 0
// Point       : 通常ノーツか、ホールドの始点か終点か
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnNoteJudged, ERhythmJudgement, Judgement, int32, LaneIndex, float, TimingError, int32, Combo, ENoteJudgePoint, Point);

UCLASS()
class ARhythmJudge : public AActor
{
	GENERATED_BODY()

public:

	ARhythmJudge();

	virtual void Tick(float DeltaTime) override;

	// 判定対象としてノーツを登録する（ノーツをSpawnした側が呼ぶ）
	void RegisterNote(ANoteActor* Note);

	// レーンが押されたときの処理（現在時刻で判定。タッチやUIボタンからも呼べる）
	UFUNCTION(BlueprintCallable, Category="Rhythm Judge")
	void PressLane(int32 LaneIndex);

	// レーンが離されたときの処理（現在時刻で判定。ホールドの終点判定に使う）
	UFUNCTION(BlueprintCallable, Category="Rhythm Judge")
	void ReleaseLane(int32 LaneIndex);

	// 入力時刻を指定して押した・離したを処理する（入力コンポーネントの Timestamp を使うとき）
	void PressLaneAt(int32 LaneIndex, double InputTime);
	void ReleaseLaneAt(int32 LaneIndex, double InputTime);

	// スコア・コンボ・体力をリセットする
	UFUNCTION(BlueprintCallable, Category="Rhythm Judge")
	void ResetResult();

	// スコア・コンボ・体力（UIやリザルトはここから読む）
	UFUNCTION(BlueprintPure, Category="Rhythm Judge")
	URhythmScoreComponent* GetScoreComponent() const { return ScoreComponent; }

	UFUNCTION(BlueprintPure, Category="Rhythm Judge")
	int32 GetCombo() const;

	UFUNCTION(BlueprintPure, Category="Rhythm Judge")
	int32 GetMaxCombo() const;

	UFUNCTION(BlueprintPure, Category="Rhythm Judge")
	int32 GetJudgementCount(ERhythmJudgement Judgement) const;

	UFUNCTION(BlueprintPure, Category="Rhythm Judge")
	bool IsGameOver() const;

	// 判定が出るたびに呼ばれる
	UPROPERTY(BlueprintAssignable, Category="Rhythm Judge")
	FOnNoteJudged OnNoteJudged;

protected:

	virtual void BeginPlay() override;

	// スコア・コンボ・体力の管理
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rhythm Judge")
	TObjectPtr<URhythmScoreComponent> ScoreComponent;

	// レーンごとのキー（要素番号 = レーン番号）。入力コンポーネント（URhythmInputComponent）と同じ D / F / J / K。
	// 入力コンポーネントが無いとき（テスト用の別レベルなど）だけ使う。あるときはキーの割り当ては入力コンポーネント側
	UPROPERTY(EditAnywhere, Category="Rhythm Judge|Input")
	TArray<FKey> LaneKeys;

	// 判定の幅（判定ラインに届く時刻から ±何秒以内か）
	UPROPERTY(EditAnywhere, Category="Rhythm Judge|Window", meta=(ClampMin=0.0, Units="s"))
	float PerfectWindow = 0.040f;

	UPROPERTY(EditAnywhere, Category="Rhythm Judge|Window", meta=(ClampMin=0.0, Units="s"))
	float GreatWindow = 0.080f;

	UPROPERTY(EditAnywhere, Category="Rhythm Judge|Window", meta=(ClampMin=0.0, Units="s"))
	float GoodWindow = 0.120f;

	// これより早く押しても無反応。これより遅れても押されなければミス
	// ホールドの終点も同じ幅で、これより早く離す・これより遅れても離さないとミス
	UPROPERTY(EditAnywhere, Category="Rhythm Judge|Window", meta=(ClampMin=0.0, Units="s"))
	float BadWindow = 0.160f;

	// 判定結果を画面左上に表示する（テスト用）
	UPROPERTY(EditAnywhere, Category="Rhythm Judge|Debug")
	bool bShowDebugText = true;

private:

	// ズレ（秒の絶対値）から判定を求める。どの幅にも入らなければ Miss
	ERhythmJudgement CalcJudgement(float AbsError) const;

	// 判定を確定させてスコア・コンボ・体力を更新し、通知する
	void ApplyJudgement(ANoteActor* Note, ERhythmJudgement Judgement, float TimingError, ENoteJudgePoint Point);

	// 体力が 0 になったとき。残りのノーツを判定対象から外し、入力を受け付けなくする
	UFUNCTION()
	void HandleGameOver();

	// 入力コンポーネントからのレーン入力（キーボード D/F/J/K・スティック弾き）
	UFUNCTION()
	void HandleLaneInput(const FLaneInput& Input);

	// 入力コンポーネントからのレーンを離した通知（キーボード・スティック）
	UFUNCTION()
	void HandleLaneRelease(const FLaneInput& Input);

	// スコア・体力をデバッグ表示する
	void ShowDebugStatus() const;

	// 始点の判定待ちのノーツ（通常・ホールド両方）
	UPROPERTY()
	TArray<TObjectPtr<ANoteActor>> ActiveNotes;

	// 押し続けている最中のホールド（キー = レーン番号）
	UPROPERTY()
	TMap<int32, TObjectPtr<AHoldNoteActor>> HoldingNotes;
};
