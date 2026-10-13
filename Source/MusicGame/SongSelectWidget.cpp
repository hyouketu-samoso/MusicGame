// Fill out your copyright notice in the Description page of Project Settings.

#include "SongSelectWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Rhythm/RhythmSettingsSubsystem.h"

namespace
{
	// 画面の難易度 ⇔ 設定置き場の難易度
	ERhythmDifficulty ToRhythmDifficulty(ESongDifficulty Difficulty)
	{
		return Difficulty == ESongDifficulty::Hard ? ERhythmDifficulty::Hard : ERhythmDifficulty::Normal;
	}

	ESongDifficulty ToSongDifficulty(ERhythmDifficulty Difficulty)
	{
		return Difficulty == ERhythmDifficulty::Hard ? ESongDifficulty::Hard : ESongDifficulty::Normal;
	}
}

URhythmSettingsSubsystem* USongSelectWidget::GetRhythmSettings() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<URhythmSettingsSubsystem>() : nullptr;
}

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
	// 難易度・速度は設定置き場（前回選んだ値・セーブされた速度）から表示する。無ければ NORMAL・速度3
	if (const URhythmSettingsSubsystem* Settings = GetRhythmSettings())
	{
		SetSelectedDifficulty(ToSongDifficulty(Settings->GetDifficulty()));
		SetSelectedSpeed(Settings->GetNoteSpeedLevel());
	}
	else
	{
		SetSelectedDifficulty(ESongDifficulty::Normal);
		SetSelectedSpeed(3);
	}

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
	SelectDifficulty(ESongDifficulty::Normal);
}

void USongSelectWidget::HandleHard()
{
	SelectDifficulty(ESongDifficulty::Hard);
}

void USongSelectWidget::SelectDifficulty(ESongDifficulty Difficulty)
{
	SetSelectedDifficulty(Difficulty);

	// 設定置き場に保存（プレイ画面の譜面選びに使う。速度は変えない）
	if (URhythmSettingsSubsystem* Settings = GetRhythmSettings())
	{
		Settings->SetDifficulty(ToRhythmDifficulty(Difficulty));
	}

	OnDifficultySelected.Broadcast(Difficulty);
}

void USongSelectWidget::SelectSpeed(int32 Speed)
{
	SetSelectedSpeed(Speed);

	// 設定置き場に保存（プレイ画面のノーツ速度に使う。自動でセーブされる）
	if (URhythmSettingsSubsystem* Settings = GetRhythmSettings())
	{
		Settings->SetNoteSpeedLevel(Speed);
	}

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