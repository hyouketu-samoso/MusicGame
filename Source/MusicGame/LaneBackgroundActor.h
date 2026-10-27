// ============================================================
// LaneBackgroundActor
//  役割 : レーンの背景UIを、3Dの世界の中に置く(ノーツより奥に表示するため)
//  方針 : UMGのウィジェットは必ず3Dより手前に描かれるので、
//         WidgetComponent(ワールド空間)で、ウィジェットを3Dの板として置く
//         ノーツより「カメラから遠い場所」に置けば、ノーツが手前に見える
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaneBackgroundActor.generated.h"

class UWidgetComponent;
class UUserWidget;

UCLASS()
class MUSICGAME_API ALaneBackgroundActor : public AActor
{
	GENERATED_BODY()

public:
	ALaneBackgroundActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	// 表示するウィジェット(レーンの画像だけを入れた WBP を指定する)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lane")
	TSubclassOf<UUserWidget> LaneWidgetClass;

	// ウィジェットを描く解像度(画像の大きさに合わせる。実際の大きさは、アクタのスケールで調整)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lane")
	FIntPoint DrawSize = FIntPoint(1920, 1080);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> WidgetComp;
};