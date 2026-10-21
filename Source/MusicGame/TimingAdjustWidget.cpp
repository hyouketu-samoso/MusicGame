#include "TimingAdjustWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "UObject/ConstructorHelpers.h"
#include "Rhythm/RhythmSettingsSubsystem.h"
#include "Input/RhythmInputComponent.h"

// ============================================================
// 生成・初期化
// ============================================================

UTimingAdjustWidget::UTimingAdjustWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 本編（RhythmFeedbackComponent）と同じ SE / Sound Class を初期値にする
	static ConstructorHelpers::FObjectFinder<USoundBase> PerfectFinder(
		TEXT("/Game/MusicGame/Sound_P/Sound_SE/SE_Notes_Perfect.SE_Notes_Perfect"));
	PerfectSE = PerfectFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundClass> SEClassFinder(
		TEXT("/Game/MusicGame/Sound_P/Sound_Goto/SCL_SE.SCL_SE"));
	SESoundClass = SEClassFinder.Object;
}

URhythmSettingsSubsystem* UTimingAdjustWidget::GetRhythmSettings() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<URhythmSettingsSubsystem>() : nullptr;
}

void UTimingAdjustWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BackButton->OnClicked.AddDynamic(this, &UTimingAdjustWidget::HandleBack);

	// 入力コンポーネントがあれば、スティックのしきい値を本編に合わせる
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const URhythmInputComponent* RhythmInput = PC->FindComponentByClass<URhythmInputComponent>())
		{
			FireThreshold = RhythmInput->FireThreshold;
			ReleaseThreshold = RhythmInput->ReleaseThreshold;
		}
	}

	// 曲選択のノーツ速度（到達時間）をそのまま使う
	if (bUseNoteSpeedSetting)
	{
		if (const URhythmSettingsSubsystem* Settings = GetRhythmSettings())
		{
			NoteTravelSec = Settings->GetNoteArrivalTime();
		}
	}

	// 拍の間隔が到達時間より短いと、ノーツが途中から急に現れるので伸ばす
	if (bExtendBeatToTravel)
	{
		BeatIntervalSec = FMath::Max(BeatIntervalSec, NoteTravelSec);
	}

	// 開いてから1拍待って、最初のノーツを流す
	StartTime = FPlatformTime::Seconds() + BeatIntervalSec;

	// 状態を初期化
	Samples.Reset();
	bLeftArmed = true;
	bRightArmed = true;
	bClosed = false;
	ConsumedBeatIndex = -1;
	ConsumedInputTime = 0.0;

	// 初回の読み込み待ちでSEが遅れないように先に読み込む
	if (PerfectSE)
	{
		UGameplayStatics::PrimeSound(PerfectSE);
	}
	if (EmptyPressSE)
	{
		UGameplayStatics::PrimeSound(EmptyPressSE);
	}

	if (LastErrorText)
	{
		LastErrorText->SetText(FText::GetEmpty());
	}
	RefreshValueText();

	BackButton->SetKeyboardFocus();
}

// ============================================================
// 毎フレームの更新
// ============================================================

void UTimingAdjustWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ゲームが止まっていてもウィジェットは動く。時刻は実時間で測る
	const double Now = FPlatformTime::Seconds();
	UpdateNoteVisual(Now);

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

	// いま表示しようとしているノーツが、入力で消されたものなら隠す
	const int32 ShownBeatIndex = bShowPast ? K : K + 1;
	if (bConsumeNoteOnInput && ShownBeatIndex == ConsumedBeatIndex)
	{
		NoteImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const double HitTime = bShowPast ? PastBeat : NextBeat;

	// 到達時間の進み具合：0 = 生成した瞬間 / 1 = 判定ラインに届いた / 1超 = 通り過ぎた
	const float Progress = static_cast<float>(1.0 - (HitTime - Now) / Travel);

	// まだ画面に現れる前
	const float AppearProgress = FMath::Clamp(NoteAppearProgress, 0.0f, 0.9f);
	if (Progress < AppearProgress)
	{
		NoteImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	// 現れ始めの位置を 0、判定ラインを 1 として、遠近感をかけた位置の進み具合を求める
	const float EasedAppear = ApplyDepthEase(AppearProgress);
	const float P = (ApplyDepthEase(Progress) - EasedAppear) / FMath::Max(1.0f - EasedAppear, KINDA_SMALL_NUMBER);

	NoteImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	NoteImage->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(NoteStartY, NoteEndY, P)));
}

float UTimingAdjustWidget::ApplyDepthEase(float Progress) const
{
	const float R = FMath::Max(NoteDepthRatio, 1.0f);

	// 通り過ぎたあとは、判定ライン到達時の速さのまま直線で進める（分母が0に近づくのを避ける）
	if (Progress >= 1.0f)
	{
		return 1.0f + R * (Progress - 1.0f);
	}

	// 3Dで等速に近づく物体の画面上の位置：奥(Progress=0)でゆっくり、手前(Progress=1)で速い。R = 1 なら等速
	return Progress / (R - (R - 1.0f) * Progress);
}

// ============================================================
// 入力
// ============================================================

FReply UTimingAdjustWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();

	// どちらのスティックか（左右どちらでもよい）
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

	// 倒しきった瞬間に1回だけ受け取り、戻すまで次は受け取らない
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
	// BackSpace = 戻る
	if (InKeyEvent.GetKey() == EKeys::BackSpace)
	{
		Close();
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

// ============================================================
// 入力の処理（ズレの計測・調整値の更新）
// ============================================================

void UTimingAdjustWidget::RegisterInput(double InputTime)
{
	// いちばん近い拍とのズレを求める（プラス = 遅れた / マイナス = 早かった）
	const double Interval = FMath::Max(BeatIntervalSec, 0.1f);
	const double Rel = InputTime - StartTime;
	const int32 BeatIndex = FMath::RoundToInt(Rel / Interval);
	const double ErrorSec = Rel - BeatIndex * Interval;

	// 本編の判定役と同じく、いま設定されている調整値を引いたズレでパーフェクトかを見る
	double OffsetSec = 0.0;
	if (const URhythmSettingsSubsystem* Settings = GetRhythmSettings())
	{
		OffsetSec = Settings->GetInputOffsetSec();
	}
	const bool bPerfect = (BeatIndex >= 0) && FMath::Abs(ErrorSec - OffsetSec) <= PerfectWindowSec;

	// 判定の範囲内か（下で「範囲外」として弾く入力と同じ条件）
	const bool bInRange = (BeatIndex >= 0) && FMath::Abs(ErrorSec) <= CaptureRangeSec;

	// その拍のノーツがすでに使われている（消えている）なら、2回目以降の入力は計測しない。
	if (bConsumeNoteOnInput && BeatIndex >= 0 && BeatIndex == ConsumedBeatIndex)
	{
		// 1回目の直後（両方のスティックを同時に倒した）なら、同時入力として何も鳴らさない。
		// 1回目のSEで足りているので、SEが二重にならず、空打ちも鳴らない
		if ((InputTime - ConsumedInputTime) <= SimultaneousInputSec)
		{
			return;
		}

		// 時間がたってからの入力（連打など）は、ノーツが消えたあとなので空打ち
		if (!bFeedbackOnlyOnPerfect)
		{
			PlayInputSE(/*bInRange=*/false);
		}
		return;
	}

	// 範囲外でも、倒したらそのノーツは消す（以後、その拍の入力は上で弾かれる）
	if (bConsumeNoteOnInput && BeatIndex >= 0)
	{
		ConsumedBeatIndex = BeatIndex;
		ConsumedInputTime = InputTime;
	}

	// SE（範囲外・最初のノーツ前の入力は、空打ちのSE）
	if (!bFeedbackOnlyOnPerfect || bPerfect)
	{
		PlayInputSE(bInRange);
	}

	// ---- ここから下は調整値の計測 ----

	// 最初のノーツが来る前の入力は計測しない
	if (BeatIndex < 0)
	{
		return;
	}

	// Bad 幅より外れた入力は計測しない
	if (FMath::Abs(ErrorSec) > CaptureRangeSec)
	{
		if (LastErrorText)
		{
			LastErrorText->SetText(FText::FromString(TEXT("範囲外")));
		}
		return;
	}

	// ズレを記録する
	const float ErrorMs = static_cast<float>(ErrorSec * 1000.0);
	Samples.Add(ErrorMs);

	if (LastErrorText)
	{
		LastErrorText->SetText(FText::FromString(FString::Printf(TEXT("%+.0fms"), ErrorMs)));
	}

	// SamplesPerAdjust 回たまったら、平均を調整値に入れて保存し、集めた数を0に戻す。
	// 次はまた0回目から数え直して、たまったらまた更新する
	if (Samples.Num() >= FMath::Max(SamplesPerAdjust, 1))
	{
		if (URhythmSettingsSubsystem* Settings = GetRhythmSettings())
		{
			Settings->SetInputOffsetMs(CalcOffsetMs());
		}
		Samples.Reset();
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

// ============================================================
// SE
// ============================================================

void UTimingAdjustWidget::PlayInputSE(bool bInRange)
{
	// 範囲内はパーフェクトのSE、範囲外（空打ち）は空打ちのSE。空打ちSEが空なら鳴らさない
	USoundBase* Sound = bInRange ? PerfectSE.Get() : EmptyPressSE.Get();
	const float SEStartTime = bInRange ? PerfectSEStartTime : EmptyPressSEStartTime;
	if (!Sound)
	{
		return;
	}

	// ポーズ中でも鳴るUI用の音として再生する。SE音量スライダーが効く
	if (UAudioComponent* Audio = UGameplayStatics::CreateSound2D(
		this, Sound, SEVolume, 1.0f, 0.0f, nullptr, /*bPersistAcrossLevelTransition=*/false, /*bAutoDestroy=*/true))
	{
		Audio->bIsUISound = true;
		if (SESoundClass)
		{
			Audio->SoundClassOverride = SESoundClass;
		}
		Audio->Play(SEStartTime);
	}
}

// ============================================================
// 閉じる
// ============================================================

void UTimingAdjustWidget::HandleBack()
{
	Close();
}

void UTimingAdjustWidget::Close()
{
	// 二重に閉じない
	if (bClosed)
	{
		return;
	}

	bClosed = true;

	// 戻るSE。ウィジェットを外す前に鳴らす（ポーズ中でも鳴るUI用の音。ウィジェットが消えても最後まで鳴る）
	if (ReturnSE)
	{
		UGameplayStatics::PlaySound2D(this, ReturnSE);
	}

	RemoveFromParent();
	OnClosed.Broadcast();
}