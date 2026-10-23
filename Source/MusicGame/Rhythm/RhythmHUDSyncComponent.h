// プレイ画面の UI（UGameMainWidget）に、実際のスコア・コンボ・体力・判定を渡す係。NoteSpawner に付いている。
//
// ・判定が出るたび : SetScore（0.00〜100.00）・SetCombo・ShowJudgement（MISS も出す）
// ・体力が変わるたび : SetHP（体力 / 最大体力）
//
// UGameMainWidget は RhythmPlayerController が作って画面に出すので、ここでは画面上から探して使う
// （見つかった時点で、いまの値をまとめて渡す）。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RhythmTypes.h"
#include "RhythmHUDSyncComponent.generated.h"

class ARhythmJudge;
class URhythmScoreComponent;
class UGameMainWidget;

UCLASS(ClassGroup=(Rhythm), meta=(BlueprintSpawnableComponent))
class URhythmHUDSyncComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	URhythmHUDSyncComponent();

	// 判定役の通知に登録する（NoteSpawner が判定役を見つけたあとに呼ぶ）
	void BindJudge(ARhythmJudge* InJudge);

	// いまのスコア・コンボ・体力を UI にまとめて渡す
	UFUNCTION(BlueprintCallable, Category="Rhythm HUD")
	void RefreshAll();

protected:

	// 判定の文字（PERFECT〜MISS）を UI に出す
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm HUD")
	bool bShowJudgementText = true;

private:

	UFUNCTION()
	void HandleNoteJudged(ERhythmJudgement Judgement, int32 LaneIndex, float TimingError, int32 Combo, ENoteJudgePoint Point);

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float Delta);

	// 画面上の UGameMainWidget（まだ無ければ探す。見つかった最初の1回だけ RefreshAll する）
	UGameMainWidget* GetWidget();

	void PushScoreAndCombo(UGameMainWidget* Widget) const;
	void PushHealth(UGameMainWidget* Widget) const;

	URhythmScoreComponent* GetScore() const;

	UPROPERTY()
	TObjectPtr<ARhythmJudge> Judge;

	UPROPERTY()
	TWeakObjectPtr<UGameMainWidget> CachedWidget;
};
