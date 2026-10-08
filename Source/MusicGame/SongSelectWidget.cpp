// Fill out your copyright notice in the Description page of Project Settings.

#include "SongSelectWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void USongSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SongPrevButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSongPrev);
	SongNextButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSongNext);
	NormalButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleNormal);
	HardButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleHard);
	SpeedButton1->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed1);
	SpeedButton2->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed2);
	SpeedButton3->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed3);
	SpeedButton4->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed4);
	SpeedButton5->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed5);
	TimingButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleTiming);
	StartButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleStart);

	SpeedButtons = { SpeedButton1, SpeedButton2, SpeedButton3, SpeedButton4, SpeedButton5 };

	// 初期状態
	bClosing = false;
	SetRenderOpacity(1.0f);
	SetRenderScale(FVector2D(1.0f, 1.0f));
	SetSelectedDifficulty(ESongDifficulty::Normal);
	SetSelectedSpeed(3);

	// パッドや矢印キーで操作できるように、最初のフォーカスをスタートに置く
	StartButton->SetKeyboardFocus();
}

void USongSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bClosing)
	{
		return;
	}

	ElapsedClose += InDeltaTime;
	const float T = FMath::Clamp(ElapsedClose / FMath::Max(FadeDuration, 0.01f), 0.0f, 1.0f);

	// 最初は速く、最後はゆっくり消える
	const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, T, 2.0f);

	SetRenderOpacity(1.0f - Eased);
	SetRenderScale(FVector2D(1.0f + ScaleUpAmount * Eased));

	if (T >= 1.0f)
	{
		bClosing = false;
		RemoveFromParent();
		OnClosed.Broadcast();
	}
}

void USongSelectWidget::SetSongInfo(const FText& Title, int32 NoteCount, float LengthSeconds)
{
	SongTitleText->SetText(Title);

	const int32 Total = FMath::Max(0, FMath::RoundToInt(LengthSeconds));
	const FString Info = FString::Printf(TEXT("%d / %02d:%02d"), NoteCount, Total / 60, Total % 60);
	SongInfoText->SetText(FText::FromString(Info));
}

void USongSelectWidget::SetSelectedDifficulty(ESongDifficulty Difficulty)
{
	NormalButton->SetBackgroundColor(Difficulty == ESongDifficulty::Normal ? SelectedColor : NormalColor);
	HardButton->SetBackgroundColor(Difficulty == ESongDifficulty::Hard ? SelectedColor : NormalColor);
}

void USongSelectWidget::SetSelectedSpeed(int32 Speed)
{
	for (int32 i = 0; i < SpeedButtons.Num(); ++i)
	{
		if (SpeedButtons[i])
		{
			SpeedButtons[i]->SetBackgroundColor((i + 1) == Speed ? SelectedColor : NormalColor);
		}
	}
}

void USongSelectWidget::CloseWithFade()
{
	if (bClosing)
	{
		return;
	}

	bClosing = true;
	ElapsedClose = 0.0f;

	// 消えている最中に、ボタンが押されないようにする(見た目は変えない)
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

// ---- ボタンのクリック ----

void USongSelectWidget::HandleSongPrev() { OnSongChangeRequested.Broadcast(-1); }
void USongSelectWidget::HandleSongNext() { OnSongChangeRequested.Broadcast(+1); }

void USongSelectWidget::HandleNormal()
{
	SetSelectedDifficulty(ESongDifficulty::Normal);
	OnDifficultySelected.Broadcast(ESongDifficulty::Normal);
}

void USongSelectWidget::HandleHard()
{
	SetSelectedDifficulty(ESongDifficulty::Hard);
	OnDifficultySelected.Broadcast(ESongDifficulty::Hard);
}

void USongSelectWidget::SelectSpeed(int32 Speed)
{
	SetSelectedSpeed(Speed);
	OnSpeedSelected.Broadcast(Speed);
}

void USongSelectWidget::HandleSpeed1() { SelectSpeed(1); }
void USongSelectWidget::HandleSpeed2() { SelectSpeed(2); }
void USongSelectWidget::HandleSpeed3() { SelectSpeed(3); }
void USongSelectWidget::HandleSpeed4() { SelectSpeed(4); }
void USongSelectWidget::HandleSpeed5() { SelectSpeed(5); }

void USongSelectWidget::HandleTiming()
{
	OnTimingAdjustRequested.Broadcast();
}

void USongSelectWidget::HandleStart()
{
	if (bClosing)
	{
		return;
	}

	OnStartRequested.Broadcast();
	CloseWithFade();
}