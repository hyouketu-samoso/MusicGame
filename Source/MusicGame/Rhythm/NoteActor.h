// ノーツ1個分のActor。出現地点から判定地点まで放物線を描いて飛ぶ。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
	 */
	void Launch(const FVector& InStartPos, const FVector& InTargetPos, float InArcHeight, float InTravelTime, const FLinearColor& InColor);

	virtual void Tick(float DeltaTime) override;

protected:

	// 見た目（球)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Note")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 判定地点を通り過ぎてから消えるまでの割合（1.0 = 判定地点、1.3 = 少し通り過ぎた所）
	UPROPERTY(EditAnywhere, Category="Note", meta=(ClampMin=1.0))
	float DestroyProgress = 1.3f;

private:

	// 経過時間の割合 T（0〜）から位置を計算する
	FVector CalcPosition(float T) const;

	FVector StartPos = FVector::ZeroVector;
	FVector TargetPos = FVector::ZeroVector;
	float ArcHeight = 0.0f;
	float TravelTime = 1.0f;

	// 出現した時刻（ワールド時間)
	double SpawnTime = 0.0;

	bool bLaunched = false;
};
