// ノーツ1個分のActor。出現地点から判定地点まで放物線を描いて飛ぶ。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RhythmTypes.h"
#include "NoteActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class ANoteActor : public AActor
{
	GENERATED_BODY()

public:

	ANoteActor();

	/*
	   飛行パラメータを設定して飛ばし始める（Spawn直後に呼ぶ）
	   @param InStartPos    出現地点（ワールド座標）
	   @param InTargetPos   判定地点（ワールド座標）
	   @param InArcHeight   山なりの高さ（cm）
	   @param InTravelTime  出現してから判定地点に届くまでの秒数
	   @param InColor       ノーツの色
	   @param InLaneIndex   レーン番号
	 */
	void Launch(const FVector& InStartPos, const FVector& InTargetPos, float InArcHeight, float InTravelTime, const FLinearColor& InColor, int32 InLaneIndex);

	virtual void Tick(float DeltaTime) override;

	// レーン番号
	int32 GetLaneIndex() const { return LaneIndex; }

	// 判定ラインに届く時刻（ワールド時間）。ホールドなら始点が届く時刻
	double GetHitTime() const { return SpawnTime + TravelTime; }

	// ホールドノーツか
	virtual bool IsHold() const { return false; }

	// true の間は判定が出るまで消えない（判定役に登録されたら true になる）
	void SetWaitForJudge(bool bWait) { bWaitForJudge = bWait; }

	// 判定が確定したときに判定役から呼ばれる。ミス以外は叩かれたのでその場で消える
	virtual void OnJudged(ERhythmJudgement Judgement, ENoteJudgePoint Point);

protected:

	// 見た目（球)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Note")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 判定地点を通り過ぎてから消えるまでの割合（1.0 = 判定地点、1.3 = 少し通り過ぎた所）
	UPROPERTY(EditAnywhere, Category="Note", meta=(ClampMin=1.0))
	float DestroyProgress = 1.3f;

	// 経過時間の割合 T から位置・消滅を更新する（ホールドは見た目が違うので上書きする）
	virtual void UpdateNote(float T);

	// 色を反映する（ホールドは複数のメッシュに塗るので上書きする）
	virtual void ApplyColor(const FLinearColor& Color);

	// 経過時間の割合 T（0〜）から位置を計算する
	FVector CalcPosition(float T) const;

	float TravelTime = 1.0f;

	int32 LaneIndex = 0;

	bool bWaitForJudge = false;

private:

	FVector StartPos = FVector::ZeroVector;
	FVector TargetPos = FVector::ZeroVector;
	float ArcHeight = 0.0f;

	// 出現した時刻（ワールド時間)
	double SpawnTime = 0.0;

	bool bLaunched = false;
};
