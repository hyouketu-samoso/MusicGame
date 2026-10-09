// Fill out your copyright notice in the Description page of Project Settings.

// ============================================================
// ResultDebugActor
//  確認用: レベルに置いて、キーを押すと仮データでリザルト画面を出す
//  RhythmPlayerController には手を入れない。確認が終わったらレベルから外す
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ResultDebugActor.generated.h"

class URhythmResultWidget;

UCLASS()
class MUSICGAME_API AResultDebugActor : public AActor
{
	GENERATED_BODY()

public:
	AResultDebugActor();

	// 出すリザルトの WBP(レベルに置いた後、詳細パネルで WBP_Result を指定する)
	UPROPERTY(EditAnywhere, Category = "ResultDebug")
	TSubclassOf<URhythmResultWidget> ResultWidgetClass;

	// リザルトを出すキー(詳細パネルで変えられる)
	UPROPERTY(EditAnywhere, Category = "ResultDebug")
	FKey DebugKey = EKeys::R;

protected:
	virtual void BeginPlay() override;

private:
	void ShowResult();

	UFUNCTION() void HandleRetry();
	UFUNCTION() void HandleTitle();
	UFUNCTION() void HandleClosed();

	UPROPERTY()
	TObjectPtr<URhythmResultWidget> ResultWidget;
};
