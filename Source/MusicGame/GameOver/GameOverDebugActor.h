// ============================================================
// GameOverDebugActor  ※デバッグ専用(コミット前に、レベルから外す)
//  役割 : 指定のキーを押すと、ゲームオーバー画面を出す
//  使い方 : レベルに置いて、Game Over Widget Class に WBP_GameOver を設定する
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "GameOverDebugActor.generated.h"

class UGameOverWidget;

UCLASS()
class MUSICGAME_API AGameOverDebugActor : public AActor
{
	GENERATED_BODY()

public:
	AGameOverDebugActor();

	virtual void Tick(float DeltaSeconds) override;

protected:
	// 出す画面(WBP_GameOver を設定)
	UPROPERTY(EditAnywhere, Category = "Debug")
	TSubclassOf<UGameOverWidget> GameOverWidgetClass;

	// 押すキー(初期値: G)
	UPROPERTY(EditAnywhere, Category = "Debug")
	FKey TriggerKey = EKeys::G;

private:
	UPROPERTY(Transient)
	TObjectPtr<UGameOverWidget> Widget;
};