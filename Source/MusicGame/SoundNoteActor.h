#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoundNoteActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class MUSICGAME_API ASoundNoteActor : public AActor
{
	GENERATED_BODY()

public:

	ASoundNoteActor();

	// ノーツの飛行を開始する
	void Launch(
		const FVector& InStartPos,
		const FVector& InTargetPos,
		float InArcHeight,
		float InTravelTime,
		const FLinearColor& InColor
	);

	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Note"
	)
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 判定ラインを通過してから消える位置
	UPROPERTY(
		EditAnywhere,
		Category = "Note",
		meta = (ClampMin = "1.0")
	)
	float DestroyProgress = 1.3f;

private:

	FVector CalcPosition(float T) const;

	FVector StartPos = FVector::ZeroVector;
	FVector TargetPos = FVector::ZeroVector;

	float ArcHeight = 0.0f;
	float TravelTime = 1.0f;

	double SpawnTime = 0.0;

	bool bLaunched = false;
};