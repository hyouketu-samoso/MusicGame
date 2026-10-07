// ホールドノーツ。始点・終点のプレートと、その間をつなぐ平たい帯でできている。
// 始点が判定ラインに届いたら押し、終点が届いたタイミングで離す。
// 押している間は始点が判定ラインに留まり、帯が吸い込まれていくように短くなる。

#pragma once

#include "CoreMinimal.h"
#include "NoteActor.h"
#include "HoldNoteActor.generated.h"

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

	virtual void Abandon() override { Super::Abandon(); bHolding = false; }

protected:

	virtual void BeginPlay() override;

	virtual void UpdateNote(float T) override;

	virtual void ApplyColor(const FLinearColor& Color) override;

	// 終点の位置（傾きは始点と同じ）。TailPlate と TailMesh はこの下に付く
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hold Note")
	TObjectPtr<USceneComponent> TailPivot;

	// 終点の仮の形
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hold Note")
	TObjectPtr<UProceduralMeshComponent> TailPlate;

	// 終点のモデル用。Static Mesh を設定すると TailPlate の代わりにこちらを表示する
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hold Note")
	TObjectPtr<UStaticMeshComponent> TailMesh;

	// 始点と終点をつなぐ平たい帯（毎フレーム曲線に沿って作り直す）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hold Note")
	TObjectPtr<UProceduralMeshComponent> Body;

	// 帯の幅（プレートの横幅に対する割合）
	UPROPERTY(EditDefaultsOnly, Category="Hold Note", meta=(ClampMin=0.01, ClampMax=1.0))
	float BodyWidthRatio = 0.5f;

private:

	// 終点 TailT → 始点 HeadT の曲線に沿って帯を作る
	void UpdateBody(float TailT, float HeadT);

	float HoldDuration = 1.0f;

	FLinearColor BaseColor = FLinearColor::White;

	// 始点が判定され、押し続けている最中か
	bool bHolding = false;

	// 帯のメッシュを一度作ったか（2回目からは頂点の更新だけ）
	bool bBodyCreated = false;
};
