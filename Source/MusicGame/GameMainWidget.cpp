// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMainWidget.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Engine/World.h"

void UGameMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 動画(Player / Source が未設定のときは、何もしない)
	PlayVideos();

	// 部品が無いときは、分かるようにログを出す(クラッシュはしない)
	// HPText と HPBar は、どちらか一方だけでも使えるので、ここでは確認しない
	if (!ScoreText || !ComboText || !JudgementText)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GameMainWidget: ScoreText / ComboText / JudgementText のどれかが WBP に無い(名前と「変数である」を確認)"));
	}

	// 初期表示
	SetScore(0);
	SetHP(100, 100);  // 最大値を 0 にしないための仮の値(本当の値は、体力を持つ人が SetHP で渡す)
	SetCombo(0);
	if (JudgementText)
	{
		JudgementText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGameMainWidget::PlayVideos()
{
	if (Player1 && Source1)
	{
		Player1->SetLooping(true);
		Player1->OpenSource(Source1);
	}
	if (Player2 && Source2)
	{
		Player2->SetLooping(true);
		Player2->OpenSource(Source2);
	}
}

void UGameMainWidget::StopVideos()
{
	if (Player1) { Player1->Close(); }
	if (Player2) { Player2->Close(); }
}

// スコアの表示を更新する(5桁のゼロ埋め。マイナスは 0 にする)
void UGameMainWidget::SetScore(int32 Score)
{
	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(
			FString::Printf(TEXT("%05d"), FMath::Max(0, Score))));
	}
}

// 体力の表示を更新する(数字とゲージ)
void UGameMainWidget::SetHP(int32 CurrentHP, int32 MaxHP)
{
	// 数字(マイナスは 0 にする)
	if (HPText)
	{
		HPText->SetText(FText::FromString(
			FString::Printf(TEXT("体力:%d"), FMath::Max(0, CurrentHP))));
	}

	// ゲージ(0.0 から 1.0 の割合で渡す。最大値が 0 以下のときは何もしない)
	if (HPBar && MaxHP > 0)
	{
		HPBar->SetPercent(FMath::Clamp(static_cast<float>(CurrentHP) / static_cast<float>(MaxHP), 0.0f, 1.0f));
	}
}

// コンボの表示を更新する(0 以下のときは、文字ごと隠す)
void UGameMainWidget::SetCombo(int32 Combo)
{
	if (!ComboText)
	{
		return;
	}

	if (Combo <= 0)
	{
		ComboText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ComboText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Combo)));
	ComboText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

// 判定を表示して、JudgementDisplaySeconds 秒後に消す
// 続けて呼ばれたときは、前のタイマーを止めて、表示し直す
void UGameMainWidget::ShowJudgement(EJudgement Judgement)
{
	if (!JudgementText)
	{
		return;
	}

	// 判定ごとの文字(デザイナーの素材が来たら、ここを差し替える)
	const TCHAR* Label = TEXT("");
	switch (Judgement)
	{
	case EJudgement::Perfect: Label = TEXT("PERFECT"); break;
	case EJudgement::Great:   Label = TEXT("GREAT");   break;
	case EJudgement::Good:    Label = TEXT("GOOD");    break;
	case EJudgement::Bad:     Label = TEXT("BAD");     break;
	case EJudgement::Miss:    Label = TEXT("MISS");    break;
	}

	JudgementText->SetText(FText::FromString(Label));
	JudgementText->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 一定時間後に消す(連続で判定が来ても、最後の判定から数える)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(JudgementTimer);
		World->GetTimerManager().SetTimer(
			JudgementTimer, this, &UGameMainWidget::HideJudgement,
			FMath::Max(JudgementDisplaySeconds, 0.05f), false);
	}
}

void UGameMainWidget::HideJudgement()
{
	if (JudgementText)
	{
		JudgementText->SetVisibility(ESlateVisibility::Collapsed);
	}
}