// Fill out your copyright notice in the Description page of Project Settings.

#include "ResultDebugActor.h"
#include "RhythmResultWidget.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AResultDebugActor::AResultDebugActor()
{
	// 毎フレームの処理は要らない
	PrimaryActorTick.bCanEverTick = false;
}

void AResultDebugActor::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	// このアクターでもキー入力を受け取れるようにして、指定キーを登録する
	EnableInput(PC);
	if (InputComponent)
	{
		InputComponent->BindKey(DebugKey, IE_Pressed, this, &AResultDebugActor::ShowResult);
	}
}

void AResultDebugActor::ShowResult()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	// クラス未指定、すでに表示中のときは何もしない
	if (!PC || !ResultWidgetClass || ResultWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResultDebug: ResultWidgetClass is not set, or already shown."));
		return;
	}

	ResultWidget = CreateWidget<URhythmResultWidget>(PC, ResultWidgetClass);
	if (!ResultWidget)
	{
		return;
	}

	// 仮のデータ(本番は、判定や得点を集計する人が入れる)
	FRhythmResultData D;
	D.SongTitle = FText::FromString(TEXT("TEST SONG"));
	D.Difficulty = ESongDifficulty::Hard;
	D.Rank = ERhythmRank::SS;
	D.Score = 99.87f;
	D.MaxCombo = 256;
	D.PerfectCount = 240;
	D.GreatCount = 12;
	D.GoodCount = 3;
	D.BadCount = 1;
	D.MissCount = 0;
	D.bFullCombo = true;
	D.bNewRecord = true;
	D.FastCount = 12;
	D.LateCount = 5;
	ResultWidget->SetResult(D);

	ResultWidget->OnRetryRequested.AddDynamic(this, &AResultDebugActor::HandleRetry);
	ResultWidget->OnTitleRequested.AddDynamic(this, &AResultDebugActor::HandleTitle);
	ResultWidget->OnClosed.AddDynamic(this, &AResultDebugActor::HandleClosed);

	ResultWidget->AddToViewport(1000);   // 他のウィジェットより手前

	// ボタンをマウスで押せるようにする
	PC->SetInputMode(FInputModeUIOnly());
	PC->bShowMouseCursor = true;
}

// いまは押されたことをログに出すだけ
void AResultDebugActor::HandleRetry() { UE_LOG(LogTemp, Log, TEXT("Result: Retry pressed")); }
void AResultDebugActor::HandleTitle() { UE_LOG(LogTemp, Log, TEXT("Result: Title pressed")); }

// フェードが終わって消えたあと: 入力を元に戻す
void AResultDebugActor::HandleClosed()
{
	ResultWidget = nullptr;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

