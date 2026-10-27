// Fill out your copyright notice in the Description page of Project Settings.

#include "SongSelectWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/UserInterfaceSettings.h"   // 青い枠線(フォーカスの枠)の設定
#include "Rhythm/RhythmSettingsSubsystem.h"

namespace
{
	// 画面の難易度 → 設定置き場の難易度
	ERhythmDifficulty ToRhythmDifficulty(ESongDifficulty Difficulty)
	{
		switch (Difficulty)
		{
		case ESongDifficulty::VeryEasy:
			return ERhythmDifficulty::VeryEasy;
		case ESongDifficulty::Easy:
			return ERhythmDifficulty::Easy;
		case ESongDifficulty::Normal:
			return ERhythmDifficulty::Normal;
		case ESongDifficulty::Hard:
			return ERhythmDifficulty::Hard;
		case ESongDifficulty::VeryHard:
			return ERhythmDifficulty::VeryHard;
		default:
			return ERhythmDifficulty::Normal;
		}
	}

	// 設定置き場の難易度 → 画面の難易度
	ESongDifficulty ToSongDifficulty(ERhythmDifficulty Difficulty)
	{
		switch (Difficulty)
		{
		case ERhythmDifficulty::VeryEasy:
			return ESongDifficulty::VeryEasy;
		case ERhythmDifficulty::Easy:
			return ESongDifficulty::Easy;
		case ERhythmDifficulty::Normal:
			return ESongDifficulty::Normal;
		case ERhythmDifficulty::Hard:
			return ESongDifficulty::Hard;
		case ERhythmDifficulty::VeryHard:
			return ESongDifficulty::VeryHard;
		default:
			return ESongDifficulty::Normal;
		}
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

	// 選んでいるボタンの青い枠線を消す
	// (エンジンの「フォーカスの枠を描くか」の設定を、この画面が出ている間だけ「描かない」にする。
	//  ini は書き換えない。画面が消えるときに、元に戻す)
	if (UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>())
	{
		if (!bFocusRuleChanged)
		{
			PrevRenderFocusRule = static_cast<uint8>(UISettings->RenderFocusRule);
			bFocusRuleChanged = true;
		}
		UISettings->RenderFocusRule = ERenderFocusRule::Never;
	}

	VeryEasyButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleVeryEasy);
	EasyButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleEasy);
	NormalButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleNormal);
	HardButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleHard);
	VeryHardButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleVeryHard);
	SpeedButton1->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed1);
	SpeedButton2->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed2);
	SpeedButton3->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed3);
	SpeedButton4->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed4);
	SpeedButton5->OnClicked.AddDynamic(this, &USongSelectWidget::HandleSpeed5);
	TimingButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleTiming);
	StartButton->OnClicked.AddDynamic(this, &USongSelectWidget::HandleStart);

	SpeedButtons = { SpeedButton1, SpeedButton2, SpeedButton3, SpeedButton4, SpeedButton5 };

	// 難易度のボタン(3〜5番目は、WBP に置いてあるときだけ使う)
	DifficultyButtons = { VeryEasyButton,EasyButton, NormalButton, HardButton,VeryHardButton };

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

	// パッドや矢印キーで操作できるように、最初のフォーカスを、選んでいる難易度のボタンに置く
	FocusDefaultButton();
}

// 画面が消えるときに、フォーカスの枠の設定を元に戻す(どんな消え方でも戻す)
void USongSelectWidget::NativeDestruct()
{
	if (bFocusRuleChanged)
	{
		if (UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>())
		{
			UISettings->RenderFocusRule = static_cast<ERenderFocusRule>(PrevRenderFocusRule);
		}
		bFocusRuleChanged = false;
	}
	Super::NativeDestruct();
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

void USongSelectWidget::SetSongTitle(const FText& Title)
{
	if (SongTitleText)
	{
		SongTitleText->SetText(Title);
	}
}

// 変更前

//void USongSelectWidget::SetSelectedDifficulty(ESongDifficulty Difficulty)
//{
//	NormalButton->SetBackgroundColor(Difficulty == ESongDifficulty::Normal ? SelectedColor : NormalColor);
//	HardButton->SetBackgroundColor(Difficulty == ESongDifficulty::Hard ? SelectedColor : NormalColor);
//}

//void USongSelectWidget::SetSelectedSpeed(int32 Speed)
//{
//	for (int32 i = 0; i < SpeedButtons.Num(); ++i)
//	{
//		if (SpeedButtons[i])
//		{
//			SpeedButtons[i]->SetBackgroundColor((i + 1) == Speed ? SelectedColor : NormalColor);
//		}
//	}
//}

void USongSelectWidget::SetSelectedDifficulty(ESongDifficulty Difficulty)
{
	CurrentDifficulty = Difficulty;   // 最初のフォーカスを置くボタンを決めるために覚えておく

	// 5つの難易度のうち、選んだものだけを「選択中」の見た目にする(無いボタンは飛ばす)
	const int32 Selected = static_cast<int32>(Difficulty);
	for (int32 i = 0; i < DifficultyButtons.Num(); ++i)
	{
		if (DifficultyButtons[i])
		{
			SetButtonToggledOn(DifficultyButtons[i], i == Selected);
		}
	}
}

void USongSelectWidget::SetSelectedSpeed(int32 Speed)
{
	for (int32 i = 0; i < SpeedButtons.Num(); ++i)
	{
		SetButtonToggledOn(SpeedButtons[i], (i + 1) == Speed);
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

void USongSelectWidget::HandleVeryEasy()
{
	SelectDifficulty(ESongDifficulty::VeryEasy);
}

void USongSelectWidget::HandleEasy()
{
	SelectDifficulty(ESongDifficulty::Easy);
}

void USongSelectWidget::HandleNormal()
{
	SelectDifficulty(ESongDifficulty::Normal);
}

void USongSelectWidget::HandleHard()
{
	SelectDifficulty(ESongDifficulty::Hard);
}

void USongSelectWidget::HandleVeryHard()
{
	SelectDifficulty(ESongDifficulty::VeryHard);
}

void USongSelectWidget::SelectDifficulty(ESongDifficulty Difficulty)
{
	SetSelectedDifficulty(Difficulty);

	// 選択した難易度を設定置き場に保存する
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

// フェード中にpadでも操作されないようにする
FReply USongSelectWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	// フェードアウト中は、パッド/キーボード操作を受け付けない
	if (bClosing)
	{
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}


// 最初にフォーカスするボタン: 選んでいる難易度のボタン(無ければ、スタートボタン)
UButton* USongSelectWidget::GetDefaultFocusButton() const
{
	const int32 Index = static_cast<int32>(CurrentDifficulty);
	if (DifficultyButtons.IsValidIndex(Index) && DifficultyButtons[Index])
	{
		return DifficultyButtons[Index];
	}
	return StartButton;
}

TSharedRef<SWidget> USongSelectWidget::GetDefaultFocusWidget()
{
	return GetDefaultFocusButton()->TakeWidget();
}

void USongSelectWidget::FocusDefaultButton()
{
	GetDefaultFocusButton()->SetKeyboardFocus();
}

FReply USongSelectWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	// マウスクリックは無視する(フォーカスが外れないように)
	return FReply::Handled();
}