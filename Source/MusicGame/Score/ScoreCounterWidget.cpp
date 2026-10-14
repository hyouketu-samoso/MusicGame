// ============================================================
// ScoreCounterWidget.cpp
//  スコアを画像の数字で表示して、変わった桁と、増えた分(+100)に動きをつける
//  注意 : UWidget に「Slot」というメンバーがあるので、変数名に Slot は使わない
// ============================================================
#include "ScoreCounterWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"

void UScoreCounterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 画像が10個そろっていないと、数字が出ないので、ログで知らせる
	bool bOk = (DigitTextures.Num() >= 10);
	for (int32 i = 0; bOk && i < 10; ++i)
	{
		bOk = (DigitTextures[i] != nullptr);
	}
	if (!bOk)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ScoreCounterWidget: DigitTextures に 0?9 の画像が10個入っていない(WBP_ScoreCounter のクラスデフォルトを確認)"));
	}

	if (!ScoreContainer)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ScoreCounterWidget: ScoreContainer(Horizontal Box)が WBP に無い(名前と「変数である」を確認)"));
		return;
	}

	BuildScoreSlots();
	ApplyScore(0, /*bAnimate=*/ false);   // 最初は「00000」を、動かさずに出す
	HideGain();
}

UTexture2D* UScoreCounterWidget::GetDigitTexture(int32 Digit) const
{
	return DigitTextures.IsValidIndex(Digit) ? DigitTextures[Digit].Get() : nullptr;
}

// 桁の部品を、左から順に NumDigits 個作る
void UScoreCounterWidget::BuildScoreSlots()
{
	if (!ScoreContainer || !WidgetTree)
	{
		return;
	}

	ScoreContainer->ClearChildren();
	DigitSlots.Reset();

	for (int32 i = 0; i < FMath::Max(NumDigits, 1); ++i)
	{
		// 枠(大きさを固定する)
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(DigitSize.X);
		Box->SetHeightOverride(DigitSize.Y);

		// 画像2枚を、重ねて置く
		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Box->SetContent(Overlay);

		auto MakeImage = [&](UImage*& OutImage)
			{
				OutImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				OutImage->SetRenderOpacity(0.0f);   // はじめは見えない
				UOverlaySlot* OverlaySlot = Overlay->AddChildToOverlay(OutImage);
				OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
				OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			};

		FScoreDigitSlot NewSlot;
		UImage* OldImg = nullptr;
		UImage* NewImg = nullptr;
		MakeImage(OldImg);
		MakeImage(NewImg);
		NewSlot.OldImage = OldImg;
		NewSlot.NewImage = NewImg;

		// 左から順に足していく(最初に足したものが、一番左になる)
		ScoreContainer->AddChild(Box);
		DigitSlots.Add(NewSlot);
	}
}

// スコアを、桁ごとにセットする
void UScoreCounterWidget::ApplyScore(int32 Score, bool bAnimate)
{
	if (DigitSlots.Num() == 0)
	{
		return;
	}

	// 桁数に収まる最大値(例: 5桁なら 99999)で止める
	int32 MaxScore = 1;
	for (int32 i = 0; i < DigitSlots.Num(); ++i)
	{
		MaxScore *= 10;
	}
	MaxScore -= 1;

	FString Text = FString::FromInt(FMath::Clamp(Score, 0, MaxScore));

	// ゼロ埋め(例: 123 → 「00123」)
	const int32 Pad = DigitSlots.Num() - Text.Len();
	if (Pad > 0)
	{
		Text = FString::ChrN(Pad, TEXT('0')) + Text;
	}

	// 変わった桁だけ、セットする
	for (int32 i = 0; i < DigitSlots.Num(); ++i)
	{
		const int32 NewDigit = Text[i] - TEXT('0');
		if (DigitSlots[i].Digit != NewDigit)
		{
			ShowDigit(DigitSlots[i], NewDigit, bAnimate);
		}
	}
}

// 桁に数字をセットする
void UScoreCounterWidget::ShowDigit(FScoreDigitSlot& DigitSlot, int32 NewDigit, bool bAnimate)
{
	if (!DigitSlot.OldImage || !DigitSlot.NewImage)
	{
		return;
	}

	UTexture2D* NewTex = GetDigitTexture(NewDigit);

	if (bAnimate)
	{
		// 古い数字 = いま出ている数字
		UTexture2D* OldTex = GetDigitTexture(DigitSlot.Digit);
		if (OldTex)
		{
			DigitSlot.OldImage->SetBrushFromTexture(OldTex);
		}
		DigitSlot.bHasOld = (OldTex != nullptr);
		DigitSlot.OldImage->SetRenderOpacity(OldTex ? 1.0f : 0.0f);
		DigitSlot.OldImage->SetRenderTranslation(FVector2D::ZeroVector);

		// 新しい数字は、見えない状態から始める(動きは NativeTick が進める)
		if (NewTex)
		{
			DigitSlot.NewImage->SetBrushFromTexture(NewTex);
		}
		DigitSlot.NewImage->SetRenderOpacity(0.0f);
		DigitSlot.NewImage->SetRenderTranslation(FVector2D(0.0f, -RollDistance));

		DigitSlot.Elapsed = 0.0f;
		DigitSlot.bAnimating = true;
	}
	else
	{
		// 動かさずに、すぐ切り替える
		if (NewTex)
		{
			DigitSlot.NewImage->SetBrushFromTexture(NewTex);
		}
		DigitSlot.NewImage->SetRenderOpacity(1.0f);
		DigitSlot.NewImage->SetRenderTranslation(FVector2D::ZeroVector);
		DigitSlot.OldImage->SetRenderOpacity(0.0f);
		DigitSlot.bHasOld = false;
		DigitSlot.bAnimating = false;
	}

	DigitSlot.Digit = NewDigit;
}

// 大きさを決めた枠の中に、画像を1枚入れる(加算分用)
USizeBox* UScoreCounterWidget::MakeFixedImage(UTexture2D* Texture, const FVector2D& Size)
{
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Box->SetWidthOverride(Size.X);
	Box->SetHeightOverride(Size.Y);

	UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (Texture)
	{
		Image->SetBrushFromTexture(Texture);
	}
	Box->SetContent(Image);
	return Box;
}

// 加算分(+ と数字)の画像を、作り直す(例: 100 → 「+100」)
void UScoreCounterWidget::BuildGainImages(int32 Delta)
{
	if (!GainContainer || !WidgetTree)
	{
		return;
	}

	GainContainer->ClearChildren();

	if (PlusTexture)
	{
		GainContainer->AddChild(MakeFixedImage(PlusTexture, GainDigitSize));
	}

	const FString Text = FString::FromInt(FMath::Max(0, Delta));
	for (int32 i = 0; i < Text.Len(); ++i)
	{
		const int32 Digit = Text[i] - TEXT('0');
		GainContainer->AddChild(MakeFixedImage(GetDigitTexture(Digit), GainDigitSize));
	}
}

void UScoreCounterWidget::HideGain()
{
	bGainActive = false;
	if (GainContainer)
	{
		GainContainer->SetRenderOpacity(0.0f);
		GainContainer->SetRenderTranslation(FVector2D::ZeroVector);
	}
}

void UScoreCounterWidget::SetScore(int32 Score)
{
	if (DigitSlots.Num() == 0)
	{
		return;
	}

	// 最初の1回は、動かさずに、そのまま出す
	if (!bInitialized)
	{
		bInitialized = true;
		CurrentScore = Score;
		ApplyScore(Score, /*bAnimate=*/ false);
		return;
	}

	if (Score == CurrentScore)
	{
		return;
	}

	const int32 Delta = Score - CurrentScore;
	CurrentScore = Score;

	// 減ったとき(やり直しなど): 動かさずに、すぐ切り替える
	if (Delta < 0)
	{
		ApplyScore(Score, /*bAnimate=*/ false);
		HideGain();
		return;
	}

	// 増えたとき: 変わった桁を動かして、増えた分(+100)を出す
	ApplyScore(Score, /*bAnimate=*/ true);

	if (GainContainer)
	{
		BuildGainImages(Delta);
		bGainActive = true;
		GainElapsed = 0.0f;
	}
}

// 毎フレーム: 動いている桁と、加算分を、少しずつ進める
void UScoreCounterWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ---- 数字: 少し下がって薄くなる → 入れ替わる → 上から戻ってくる ----
	const float Split = 0.35f;   // 前半(下がる)と後半(戻る)の境目

	for (FScoreDigitSlot& DigitSlot : DigitSlots)
	{
		if (!DigitSlot.bAnimating || !DigitSlot.OldImage || !DigitSlot.NewImage)
		{
			continue;
		}

		DigitSlot.Elapsed += InDeltaTime;
		const float T = FMath::Clamp(DigitSlot.Elapsed / FMath::Max(RollDuration, 0.01f), 0.0f, 1.0f);

		if (T < Split)
		{
			// 前半: 古い数字が、下がりながら消える
			const float P = T / Split;
			if (DigitSlot.bHasOld)
			{
				DigitSlot.OldImage->SetRenderTranslation(FVector2D(0.0f, RollDistance * P));
				DigitSlot.OldImage->SetRenderOpacity(1.0f - P);
			}
		}
		else
		{
			// 後半: 新しい数字が、上から戻ってきて、はっきり見える
			const float P = (T - Split) / (1.0f - Split);
			const float E = FMath::InterpEaseOut(0.0f, 1.0f, P, 2.0f);
			DigitSlot.OldImage->SetRenderOpacity(0.0f);
			DigitSlot.NewImage->SetRenderTranslation(FVector2D(0.0f, -RollDistance * (1.0f - E)));
			DigitSlot.NewImage->SetRenderOpacity(E);
		}

		if (T >= 1.0f)
		{
			DigitSlot.bAnimating = false;
			DigitSlot.OldImage->SetRenderOpacity(0.0f);
			DigitSlot.NewImage->SetRenderTranslation(FVector2D::ZeroVector);
			DigitSlot.NewImage->SetRenderOpacity(1.0f);
		}
	}

	// ---- 加算分: 数字の裏から滑り出る → しばらく出す → 消える ----
	if (bGainActive && GainContainer)
	{
		GainElapsed += InDeltaTime;

		const float Ts = FMath::Clamp(GainElapsed / FMath::Max(GainSlideDuration, 0.01f), 0.0f, 1.0f);
		const float E = FMath::InterpEaseOut(0.0f, 1.0f, Ts, 2.0f);

		float Opacity = E;
		const float X = -GainSlideDistance * (1.0f - E);   // 左(数字の裏)から、元の位置へ

		// 出しておく時間が決まっているときは、そのあと消す
		if (GainHoldSeconds > 0.0f)
		{
			const float FadeStart = GainSlideDuration + GainHoldSeconds;
			if (GainElapsed > FadeStart)
			{
				const float F = FMath::Clamp(
					(GainElapsed - FadeStart) / FMath::Max(GainFadeOutSeconds, 0.01f), 0.0f, 1.0f);
				Opacity = 1.0f - F;
				if (F >= 1.0f)
				{
					HideGain();
					return;
				}
			}
		}

		GainContainer->SetRenderTranslation(FVector2D(X, 0.0f));
		GainContainer->SetRenderOpacity(Opacity);
	}
}