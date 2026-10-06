// 判定役のActor。レーンのキー入力を受け取り、飛んでいるノーツとのタイミングのズレから判定を決める。
// コンボ数・判定ごとの回数もここで管理する。
// ホールドノーツは「始点を押したとき」と「終点で離したとき」の2回判定する。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "RhythmTypes.h"
#include "RhythmJudge.generated.h"

class ANoteActor;
class AHoldNoteActor;

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

	// レーンが押されたときの処理（キー入力以外、タッチやUIボタンからも呼べる）
	UFUNCTION(BlueprintCallable, Category="Rhythm Judge")
	void PressLane(int32 LaneIndex);

	// レーンが離されたときの処理（ホールドの終点判定に使う）
	UFUNCTION(BlueprintCallable, Category="Rhythm Judge")
	void ReleaseLane(int32 LaneIndex);

	// 判定・コンボをリセットする
	UFUNCTION(BlueprintCallable, Category="Rhythm Judge")
	void ResetResult();

	UFUNCTION(BlueprintPure, Category="Rhythm Judge")
	int32 GetCombo() const { return Combo; }

	UFUNCTION(BlueprintPure, Category="Rhythm Judge")
	int32 GetMaxCombo() const { return MaxCombo; }

	UFUNCTION(BlueprintPure, Category="Rhythm Judge")
	int32 GetJudgementCount(ERhythmJudgement Judgement) const;

	// 判定が出るたびに呼ばれる
	UPROPERTY(BlueprintAssignable, Category="Rhythm Judge")
	FOnNoteJudged OnNoteJudged;

protected:

	virtual void BeginPlay() override;

	// レーンごとの入力キー（要素番号 = レーン番号）
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

	// 判定を確定させてコンボを更新し、通知する
	void ApplyJudgement(ANoteActor* Note, ERhythmJudgement Judgement, float TimingError, ENoteJudgePoint Point);

	// 始点の判定待ちのノーツ（通常・ホールド両方）
	UPROPERTY()
	TArray<TObjectPtr<ANoteActor>> ActiveNotes;

	// 押し続けている最中のホールド（キー = レーン番号）
	UPROPERTY()
	TMap<int32, TObjectPtr<AHoldNoteActor>> HoldingNotes;

	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Judge|Result")
	int32 Combo = 0;

	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Judge|Result")
	int32 MaxCombo = 0;

	// 判定ごとの回数（ERhythmJudgement の順）。ホールドは始点と終点で2回数える
	UPROPERTY(VisibleInstanceOnly, Category="Rhythm Judge|Result")
	TArray<int32> JudgementCounts;
};
