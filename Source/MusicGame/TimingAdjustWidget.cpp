#include "TimingAdjustWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Rhythm/RhythmSettingsSubsystem.h"
#include "Input/RhythmInputComponent.h"

URhythmSettingsSubsystem* UTimingAdjustWidget::GetRhythmSettings() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<URhythmSettingsSubsystem>() : nullptr;
}

void UTimingAdjustWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BackButton->OnClicked.AddDynamic(this, &UTimingAdjustWidget::HandleBack);

	// 入力コンポーネントがあれば、スティックのしきい値を合わせる
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const URhythmInputComponent* RhythmInput = PC->FindComponentByClass<URhythmInputComponent>())
		{
			FireThreshold = RhythmInput->FireThreshold;
			ReleaseThreshold = RhythmInput->ReleaseThreshold;
		}
	}

	// 開いてから1拍待って、最初のノーツを流す
	StartTime = FPlatformTime::Seconds() + BeatIntervalSec;

	Samples.Reset();
	bLeftArmed = true;
	bRightArmed = true;
	bClosed = false;

	if (LastErrorText)
	{
		LastErrorText->SetText(FText::GetEmpty());
	}
	RefreshValueText();

	BackButton->SetKeyboardFocus();
}

void UTimingAdjustWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ゲームが止まっていてもウィジェットは動く。時刻は実時間で測る
	UpdateNoteVisual(FPlatformTime::Seconds());

	// フォーカス中は色を変えて、選ばれていることが分かるようにする
	BackButton->SetBackgroundColor(BackButton->HasKeyboardFocus() ? SelectedColor : NormalColor);
}

void UTimingAdjustWidget::UpdateNoteVisual(double Now)
{
	if (!NoteImage)
	{
		return;
	}

	const double Interval = FMath::Max(BeatIntervalSec, 0.1f);
	const double Travel = FMath::Max(NoteTravelSec, 0.1f);
	const double Rel = Now - StartTime;

	// 通り過ぎた直近の拍(K)と次の拍
	const int32 K = FMath::FloorToInt(Rel / Interval);
	const double PastBeat = StartTime + K * Interval;
	const double NextBeat = PastBeat + Interval;

	// 通り過ぎた直後は、そのノーツを少しの間見せる
	const bool bShowPast = (K >= 0) && ((Now - PastBeat) < PassVisibleSec);
	const double HitTime = bShowPast ? PastBeat : NextBeat;

	// 0 = 出現位置 / 1 = 判定ライン / 1超 = 通り過ぎた
	const float P = static_cast<float>(1.0 - (HitTime - Now) / Travel);

	if (P < 0.0f)
	{
		NoteImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	NoteImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	NoteImage->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(NoteStartY, NoteEndY, P)));
}

FReply UTimingAdjustWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();

	bool* bArmed = nullptr;
	if (Key == EKeys::Gamepad_LeftX)
	{
		bArmed = &bLeftArmed;
	}
	else if (Key == EKeys::Gamepad_RightX)
	{
		bArmed = &bRightArmed;
	}
	else
	{
		return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
	}

	// 左右どちらでもよい。倒しきった瞬間に1回だけ受け取り、戻すまで受け取らない
	const float Abs = FMath::Abs(InAnalogEvent.GetAnalogValue());
	if (*bArmed && Abs >= FireThreshold)
	{
		*bArmed = false;
		RegisterInput(FPlatformTime::Seconds());
	}
	else if (!*bArmed && Abs <= ReleaseThreshold)
	{
		*bArmed = true;
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

FReply UTimingAdjustWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// 下(A) = 戻る。ボタンの標準動作より先に受け取り、ボタンを押した扱いにさせない
	if (Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (!InKeyEvent.IsRepeat())
		{
			Close();
		}
		return FReply::Handled();
	}

	// 右(B) = 決定。フォーカス中の「戻る」ボタンを押す
	if (Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (!InKeyEvent.IsRepeat())
		{
			if (BackButton && BackButton->HasKeyboardFocus())
			{
				BackButton->OnClicked.Broadcast();
			}
			else
			{
				Close();   // フォーカスが外れていても、確実に戻れるようにする
			}
		}
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UTimingAdjustWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// BackSpace = 戻る
	if (Key == EKeys::BackSpace)
	{
		Close();
		return FReply::Handled();
	}

	// テスト用のキー入力（押しっぱなしは無視）
	if (bAcceptKeyboard && !InKeyEvent.IsRepeat()
		&& (Key == EKeys::D || Key == EKeys::F || Key == EKeys::J || Key == EKeys::K))
	{
		RegisterInput(FPlatformTime::Seconds());
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UTimingAdjustWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	// ウィジェット自身がフォーカスを受けたら、戻るボタンに渡す
	if (BackButton)
	{
		return FReply::Handled().SetUserFocus(BackButton->TakeWidget(), InFocusEvent.GetCause());
	}

	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

void UTimingAdjustWidget::RegisterInput(double InputTime)
{
	// いちばん近い拍とのズレを求める（プラス = 遅れた / マイナス = 早かった）
	const double Interval = FMath::Max(BeatIntervalSec, 0.1f);
	const double Rel = InputTime - StartTime;
	const int32 BeatIndex = FMath::RoundToInt(Rel / Interval);

	// 最初のノーツが来る前の入力は無視
	if (BeatIndex < 0)
	{
		return;
	}

	const double ErrorSec = Rel - BeatIndex * Interval;

	// Bad 幅より外れた入力は無視
	if (FMath::Abs(ErrorSec) > CaptureRangeSec)
	{
		if (LastErrorText)
		{
			LastErrorText->SetText(FText::FromString(TEXT("範囲外")));
		}
		return;
	}

	const float ErrorMs = static_cast<float>(ErrorSec * 1000.0);

	Samples.Add(ErrorMs);
	while (Samples.Num() > FMath::Max(MaxSamples, 1))
	{
		Samples.RemoveAt(0);
	}

	if (LastErrorText)
	{
		LastErrorText->SetText(FText::FromString(FString::Printf(TEXT("%+.0fms"), ErrorMs)));
	}

	// 十分たまったら調整値を更新して保存
	if (Samples.Num() >= MinSamplesToApply)
	{
		if (URhythmSettingsSubsystem* Settings = GetRhythmSettings())
		{
			Settings->SetInputOffsetMs(CalcOffsetMs());
		}
		RefreshValueText();
	}
}

int32 UTimingAdjustWidget::CalcOffsetMs() const
{
	if (Samples.Num() == 0)
	{
		return 0;
	}

	TArray<float> Sorted = Samples;
	Sorted.Sort();

	// 外れ値の影響を減らすため、最小・最大を1つずつ除く
	int32 First = 0;
	int32 Last = Sorted.Num() - 1;
	if (bTrimOutliers && Sorted.Num() >= 5)
	{
		++First;
		--Last;
	}

	double Sum = 0.0;
	for (int32 i = First; i <= Last; ++i)
	{
		Sum += Sorted[i];
	}

	return FMath::RoundToInt(static_cast<float>(Sum / (Last - First + 1)));
}

void UTimingAdjustWidget::RefreshValueText()
{
	int32 OffsetMs = 0;
	if (const URhythmSettingsSubsystem* Settings = GetRhythmSettings())
	{
		OffsetMs = Settings->GetInputOffsetMs();
	}

	// 0 のときは「0ms」、それ以外は「+23ms」「-15ms」
	const FString Text = (OffsetMs == 0)
		? FString(TEXT("0ms"))
		: FString::Printf(TEXT("%+dms"), OffsetMs);

	AdjustValueText->SetText(FText::FromString(Text));
}

void UTimingAdjustWidget::HandleBack()
{
	Close();
}

void UTimingAdjustWidget::Close()
{
	if (bClosed)
	{
		return;
	}

	bClosed = true;
	RemoveFromParent();
	OnClosed.Broadcast();
}