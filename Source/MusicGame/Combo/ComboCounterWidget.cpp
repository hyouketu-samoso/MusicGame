// ============================================================
// ComboCounterWidget.cpp
//  コンボの数字を、画像で表示して、変わるときに「縦に流れる」動きをつける
//  注意 : UWidget に「Slot」というメンバーがあるので、変数名に Slot は使わない
// ============================================================
#include "ComboCounterWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"

void UComboCounterWidget::NativeConstruct()
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
			TEXT("ComboCounterWidget: DigitTextures に 0?9 の画像が10個入っていない(WBP_ComboCounter のクラスデフォルトを確認)"));
	}

	// 最初は、コンボが0なので隠しておく
	SetVisibility(ESlateVisibility::Collapsed);
}

UTexture2D* UComboCounterWidget::GetDigitTexture(int32 Digit) const
{
	return DigitTextures.IsValidIndex(Digit) ? DigitTextures[Digit].Get() : nullptr;
}

// 桁の部品を1つ作って、左端に足す
void UComboCounterWidget::AddSlotAtLeft()
{
	if (!DigitContainer || !WidgetTree)
	{
		return;
	}

	FComboDigitSlot NewSlot;

	// 枠(大きさを固定して、はみ出た部分を切り取る)
	NewSlot.Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	NewSlot.Box->SetWidthOverride(DigitSize.X);
	NewSlot.Box->SetHeightOverride(DigitSize.Y);
	NewSlot.Box->SetClipping(EWidgetClipping::ClipToBounds);

	// 画像2枚を、重ねて置く
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	NewSlot.Box->SetContent(Overlay);

	auto MakeImage = [&](UImage*& OutImage)
		{
			OutImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			OutImage->SetRenderOpacity(0.0f);   // はじめは見えない
			UOverlaySlot* OverlaySlot = Overlay->AddChildToOverlay(OutImage);
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		};
	UImage* OldImg = nullptr;
	UImage* NewImg = nullptr;
	MakeImage(OldImg);
	MakeImage(NewImg);
	NewSlot.OldImage = OldImg;
	NewSlot.NewImage = NewImg;

	// 新しい桁を、一番左にする(桁が増えたとき、新しい桁は左に出る)
	// 注意 : InsertChildAt は、動いている画面では、画面の順番が変わらないことがある。
	//        そこで、いったん全部を外して、左から順に入れ直す
	TArray<FComboDigitSlot> OldSlots = MoveTemp(DigitSlots);
	DigitContainer->ClearChildren();

	DigitSlots.Reset();
	DigitSlots.Add(NewSlot);
	DigitSlots.Append(OldSlots);

	for (FComboDigitSlot& DigitSlot : DigitSlots)
	{
		DigitContainer->AddChild(DigitSlot.Box);
	}
}

// 桁に数字をセットする
void UComboCounterWidget::ShowDigit(FComboDigitSlot& DigitSlot, int32 NewDigit, bool bAnimate)
{
	if (!DigitSlot.OldImage || !DigitSlot.NewImage)
	{
		return;
	}

	UTexture2D* NewTex = GetDigitTexture(NewDigit);

	if (bAnimate)
	{
		// 古い数字 = いま出ている数字。新しい桁(まだ何も出ていない)のときは、古い方は出さない
		UTexture2D* OldTex = GetDigitTexture(DigitSlot.Digit);
		if (OldTex)
		{
			DigitSlot.OldImage->SetBrushFromTexture(OldTex);
		}
		DigitSlot.OldImage->SetRenderOpacity(OldTex ? 1.0f : 0.0f);
		DigitSlot.OldImage->SetRenderTranslation(FVector2D::ZeroVector);

		// 新しい数字は、見えない状態から始める(動きは NativeTick が進める)
		if (NewTex)
		{
			DigitSlot.NewImage->SetBrushFromTexture(NewTex);
		}
		DigitSlot.NewImage->SetRenderOpacity(0.0f);
		DigitSlot.NewImage->SetRenderTranslation(FVector2D(0.0f, DigitSize.Y * RollDistanceRatio));

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
		DigitSlot.bAnimating = false;
	}

	DigitSlot.Digit = NewDigit;
}

void UComboCounterWidget::SetCombo(int32 Combo)
{
	if (!DigitContainer)
	{
		return;
	}

	// 0 以下: 桁を全部片づけて、隠す(次にコンボが始まったら、作り直す)
	if (Combo <= 0)
	{
		DigitContainer->ClearChildren();
		DigitSlots.Reset();
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	const FString Text = FString::FromInt(Combo);   // 例: 128 → "128"
	const int32 Count = Text.Len();

	// 桁が減ったとき(例: 100 → 99)は、いったん作り直して、動かさずに表示する
	const bool bRebuild = (Count < DigitSlots.Num());
	if (bRebuild)
	{
		DigitContainer->ClearChildren();
		DigitSlots.Reset();
	}

	// 桁が足りないときは、左に足す(新しい桁は、下から出てくる動きになる)
	while (DigitSlots.Num() < Count)
	{
		AddSlotAtLeft();
	}

	// 桁ごとに、数字が変わったところだけ動かす
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 NewDigit = Text[i] - TEXT('0');
		if (DigitSlots[i].Digit != NewDigit)
		{
			ShowDigit(DigitSlots[i], NewDigit, /*bAnimate=*/ !bRebuild);
		}
	}
}

// 毎フレーム: 動いている桁を、少しずつ進める
void UComboCounterWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float Distance = DigitSize.Y * RollDistanceRatio;

	for (FComboDigitSlot& DigitSlot : DigitSlots)
	{
		if (!DigitSlot.bAnimating || !DigitSlot.OldImage || !DigitSlot.NewImage)
		{
			continue;
		}

		DigitSlot.Elapsed += InDeltaTime;
		const float T = FMath::Clamp(DigitSlot.Elapsed / FMath::Max(RollDuration, 0.01f), 0.0f, 1.0f);

		// 最初は速く、最後はゆっくり止まる
		const float E = FMath::InterpEaseOut(0.0f, 1.0f, T, 2.0f);

		// 古い数字: 上に流れながら、消える
		DigitSlot.OldImage->SetRenderTranslation(FVector2D(0.0f, -Distance * E));
		DigitSlot.OldImage->SetRenderOpacity(1.0f - E);

		// 新しい数字: 下から上がってきて、はっきり見える
		DigitSlot.NewImage->SetRenderTranslation(FVector2D(0.0f, Distance * (1.0f - E)));
		DigitSlot.NewImage->SetRenderOpacity(E);

		if (T >= 1.0f)
		{
			DigitSlot.bAnimating = false;
			DigitSlot.OldImage->SetRenderOpacity(0.0f);
			DigitSlot.NewImage->SetRenderTranslation(FVector2D::ZeroVector);
			DigitSlot.NewImage->SetRenderOpacity(1.0f);
		}
	}
}