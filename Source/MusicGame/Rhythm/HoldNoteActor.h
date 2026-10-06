// ホールドノーツ。始点（球）・終点（球）とその間をつなぐ帯でできている。
// 始点が判定ラインに届いたら押し、終点が届いたタイミングで離す。
// 押している間は始点が判定ラインに留まり、帯が吸い込まれていくように短くなる。

#pragma once

#include "CoreMinimal.h"
#include "NoteActor.h"
#include "HoldNoteActor.generated.h"

class UMaterialInstanceDynamic;

UCLASS()
class AHoldNoteActor : public ANoteActor
{
	GENERATED_BODY()

public:

	AHoldNoteActor();

	// 押し続ける長さ（秒）を設定する（Launch より前に呼ぶ）
	void SetHoldDuration(float InDuration) { HoldDuration = FMath::Max(InDuration, 0.0f); }

	float GetHoldDuration() const { return HoldDuration; }

	// 終点が判定ラインに届く時刻（ワールド時間）
	double GetEndTime() const { return GetHitTime() + HoldDuration; }

	virtual bool IsHold() const override { return true; }

	virtual void OnJudged(ERhythmJudgement Judgement, ENoteJudgePoint Point) override;

protected:

	virtual void UpdateNote(float T) override;

	virtual void ApplyColor(const FLinearColor& Color) override;

	// 終点の球
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hold Note")
	TObjectPtr<UStaticMeshComponent> TailMesh;

	// 始点と終点をつなぐ帯（短い円柱を曲線に沿って並べる）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hold Note")
	TArray<TObjectPtr<UStaticMeshComponent>> BodySegments;

	// 帯の太さ（円柱の直径 100cm に対する倍率）
	UPROPERTY(EditAnywhere, Category="Hold Note", meta=(ClampMin=0.01))
	float BodyThickness = 0.25f;

private:

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BodyMID;

	float HoldDuration = 1.0f;

	FLinearColor BaseColor = FLinearColor::White;

	// 始点が判定され、押し続けている最中か
	bool bHolding = false;
};
