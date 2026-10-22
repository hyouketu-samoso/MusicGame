// Fill out your copyright notice in the Description page of Project Settings.

// ============================================================
// RhythmResultWidget.cpp
//  リザルト画面の処理。
//  - SetResult() で受け取ったデータを、WBP側の文字に流し込む
//  - リトライ / タイトルのボタンは「押された」通知を出して、フェードアウトのあとにレベルを移動する
//    (タイトル = 曲選択のレベルへ / リトライ = いまのレベルを開き直す)
//  - 表示するときにフェードイン、消すときにフェードアウトする
//  - 表示している間はゲームを止め(ポーズ)、消えるときに解除する
// ============================================================
#include "RhythmResultWidget.h"
#include "Components/Button.h"        // UButton を使うために必要
#include "Components/HorizontalBox.h"   // 数字を並べる入れ物
#include "Blueprint/WidgetTree.h"        // 数字の画像をその場で作るのに必要
#include "Components/Image.h"         // UImage を使うために必要
#include "Engine/Texture2D.h"          // UTexture2D を使うために必要
#include "Components/TextBlock.h"     // UTextBlock を使うために必要
#include "Kismet/GameplayStatics.h"  // SetGamePaused を使うために必要
#include "GameFramework/PlayerController.h"   // 入力モードの切り替え(FInputModeUIOnly)に必要
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"   // EKeys(ゲームパッドのボタン名)
#include "GameSessionSubsystem.h"
#include "UObject/UnrealType.h"

// ボタンを「フォーカスしない」にする(UE標準の青い枠線が出なくなる)
// (エンジンのバージョンでプロパティ名が違っても動くよう、名前で探して設定する)
static void SetResultButtonNotFocusable(UButton* Button)
{
	if (!Button)
	{
		return;
	}
	for (const TCHAR* Name : { TEXT("IsFocusable"), TEXT("bIsFocusable") })
	{
		if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(UButton::StaticClass(), Name))
		{
			Prop->SetPropertyValue_InContainer(Button, false);
			return;
		}
	}
}

// このファイルの中だけで使う小さな補助関数
namespace
{
	// ランクの enum を、画面に出す文字に変える
	const TCHAR* RankToString(ERhythmRank Rank)
	{
		switch (Rank)
		{
		case ERhythmRank::SS: return TEXT("SS");
		case ERhythmRank::S:  return TEXT("S");
		case ERhythmRank::A:  return TEXT("A");
		case ERhythmRank::B:  return TEXT("B");
		case ERhythmRank::C:  return TEXT("C");
		default:              return TEXT("D");
		}
	}

}

// WBP側に必要な部品が全部あるかを調べる
// 1つでも無い(NULL)と、使った瞬間にクラッシュするので、その前にここで止める
bool URhythmResultWidget::HasAllParts() const
{
	// 文字や画像の部品は、無くても動く(画像だけにして、文字を消してもよい)
	// 必ず要るのは、ボタンだけ
	return RetryButton && TitleButton;
}

// ウィジェットが作られて、画面に出る準備ができたときに1回だけ呼ばれる
void URhythmResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 部品が足りないときは、ログを出して何もしない(クラッシュ防止)
	// 原因の多くは「親クラスの指定ミス」か「部品の名前の違い・変数チェック漏れ」
	if (!HasAllParts())
	{
		UE_LOG(LogTemp, Error,
			TEXT("RhythmResultWidget: widget parts are missing. Check that WBP_Result is set and part names match."));
		return;
	}

	// ボタンが押されたときに呼ぶ関数を登録する
	// (AddDynamic で登録する関数には、ヘッダーで UFUNCTION が必要)
	RetryButton->OnClicked.AddDynamic(this, &URhythmResultWidget::HandleRetry);
	TitleButton->OnClicked.AddDynamic(this, &URhythmResultWidget::HandleTitle);

	// マウスを乗せたら、選択がそのボタンに移る
	RetryButton->OnHovered.AddDynamic(this, &URhythmResultWidget::HandleRetryHovered);
	TitleButton->OnHovered.AddDynamic(this, &URhythmResultWidget::HandleTitleHovered);

	// ボタンはフォーカスさせない(青い枠線を出さない)。選択は、コードで管理する
	SetResultButtonNotFocusable(RetryButton);
	SetResultButtonNotFocusable(TitleButton);
	SetIsFocusable(true);   // 代わりに、このウィジェット自身がキー入力を受け取る
	SelectedIndex = 0;

	// 表示のはじまりは透明にして、NativeTick でだんだん見えるようにする
	State = EState::FadingIn;
	ElapsedFade = 0.0f;
	SetRenderOpacity(0.0f);

	// キーボードやパッドの操作(フォーカス)は、最初の NativeTick で設定する
	// (画面に出たあとでないと、フォーカスが付かないことがあるため)

	// 表示している間は、ゲームを止める(ウィジェットはポーズ中も動く)
	if (bPauseGameWhileShown && GetWorld())
	{
		UGameplayStatics::SetGamePaused(this, true);
		bPausedByThis = true;
	}
}

// 画面から外れるときに呼ばれる。どんな消え方でも、ポーズを必ず戻す
void URhythmResultWidget::NativeDestruct()
{
	RestoreInputMode();   // 変えた入力モードが残っていたら戻す(通常は、消える前に戻してある)
	ReleasePause();
	Super::NativeDestruct();
}

// このウィジェットがかけたポーズだけを解除する
// (他の誰かがかけたポーズを、勝手に解除しないため)
void URhythmResultWidget::ReleasePause()
{
	if (bPausedByThis && GetWorld())
	{
		UGameplayStatics::SetGamePaused(this, false);
	}
	bPausedByThis = false;
}

// スコアからランクを決める(下限以上で、一番上のランクになる)
ERhythmRank URhythmResultWidget::ScoreToRank(float Score) const
{
	const float S = FMath::Clamp(Score, 0.0f, 100.0f);
	if (S >= RankThresholdSS) { return ERhythmRank::SS; }
	if (S >= RankThresholdS) { return ERhythmRank::S; }
	if (S >= RankThresholdA) { return ERhythmRank::A; }
	if (S >= RankThresholdB) { return ERhythmRank::B; }
	if (S >= RankThresholdC) { return ERhythmRank::C; }
	return ERhythmRank::D;
}

// 画像と文字の切り替え(デザイナーの素材が来る前は文字、来たあとは画像、をコード変更なしで切り替えられる)
void URhythmResultWidget::ShowImageOrText(UImage* Image, UTexture2D* Texture, UTextBlock* Text, bool bShow)
{
	if (!bShow)
	{
		if (Image) { Image->SetVisibility(ESlateVisibility::Collapsed); }
		if (Text) { Text->SetVisibility(ESlateVisibility::Collapsed); }
		return;
	}

	if (Image && Texture)
	{
		// bMatchSize を false にして、WBP で決めた大きさのまま、画像だけ差し替える
		Image->SetBrushFromTexture(Texture, false);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (Text) { Text->SetVisibility(ESlateVisibility::Collapsed); }
	}
	else
	{
		if (Image) { Image->SetVisibility(ESlateVisibility::Collapsed); }
		if (Text) { Text->SetVisibility(ESlateVisibility::HitTestInvisible); }
	}
}

// ゲームパッド操作の準備
//  ・入力を、UI 操作に切り替える(ゲーム操作のままだと、十字キーや×ボタンが、ボタンに届かない)
//  ・ボタンにはフォーカスを置かない(選択は SelectedIndex で管理)
void URhythmResultWidget::SetupGamepadFocus()
{
	bFocusSetup = true;

	APlayerController* PC = GetOwningPlayer();
	if (!PC || !RetryButton)
	{
		return;
	}

	// 戻すときのために、変える前のカーソルの表示を覚えておく
	bPrevShowCursor = PC->bShowMouseCursor;

	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(TakeWidget());   // ボタンではなく、このウィジェット自身に入力を集める
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = true;   // マウスでも押せるようにする
	bInputModeChanged = true;
}

// このウィジェットが変えた入力モードを、ゲーム操作に戻す(変えていないときは、何もしない)
void URhythmResultWidget::RestoreInputMode()
{
	if (!bInputModeChanged)
	{
		return;
	}
	bInputModeChanged = false;

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = bPrevShowCursor;
	}
}

// 選んでいるボタンを、大きく・色を変えて見せる(どちらが選ばれているか分かるように)
// 十字キー・左スティックの移動と、○ボタンの決定は、NativeOnPreviewKeyDown で処理している
void URhythmResultWidget::UpdateFocusVisual()
{
	if (!RetryButton || !TitleButton)
	{
		return;
	}

	const bool bRetryFocus = (SelectedIndex == 0);
	const bool bTitleFocus = (SelectedIndex == 1);
	const int32 FocusIndex = SelectedIndex;

	// 変わったときだけ、見た目を更新する(毎フレーム触ると、無駄に描き直しになる)
	if (FocusIndex != LastFocusIndex)
	{
		LastFocusIndex = FocusIndex;

		auto Apply = [this](UButton* Button, bool bFocused)
			{
				Button->SetRenderScale(bFocused ? FVector2D(FocusedScale, FocusedScale) : FVector2D(1.0f, 1.0f));
				Button->SetColorAndOpacity(bFocused ? FocusedColor : NormalColor);
			};
		Apply(RetryButton, bRetryFocus);
		Apply(TitleButton, bTitleFocus);
	}

}

// 選択を動かす(2つしかないので、端で止める)
void URhythmResultWidget::MoveSelection(int32 Direction)
{
	SelectedIndex = FMath::Clamp(SelectedIndex + Direction, 0, 1);
}

// マウスを乗せたとき
void URhythmResultWidget::HandleRetryHovered() { SelectedIndex = 0; }
void URhythmResultWidget::HandleTitleHovered() { SelectedIndex = 1; }

// 毎フレーム呼ばれる。フェードの進み具合をここで更新する
void URhythmResultWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ゲームパッド操作の準備(最初の1回)と、選んでいるボタンの見た目の更新
	if (HasAllParts())
	{
		if (!bFocusSetup)
		{
			SetupGamepadFocus();
		}
		UpdateFocusVisual();
	}

	// 表示が終わって落ち着いているときは、何もしない
	if (State == EState::Shown)
	{
		return;
	}

	// フェードを始めてからの経過時間を進める
	ElapsedFade += InDeltaTime;

	if (State == EState::FadingIn)
	{
		// T は 0.0(開始)から 1.0(終了)。0 で割らないよう、秒数の最小値を決めておく
		const float T = FMath::Clamp(ElapsedFade / FMath::Max(FadeInDuration, 0.01f), 0.0f, 1.0f);

		// 最初は速く、最後はゆっくり見えてくる動きにする
		SetRenderOpacity(FMath::InterpEaseOut(0.0f, 1.0f, T, 2.0f));

		if (T >= 1.0f)
		{
			State = EState::Shown;   // 完全に見えたので、以降は Tick で何もしない
		}
	}
	else // EState::FadingOut
	{
		const float T = FMath::Clamp(ElapsedFade / FMath::Max(FadeOutDuration, 0.01f), 0.0f, 1.0f);

		// 見えている状態(1.0)から透明(0.0)に向かう
		SetRenderOpacity(1.0f - FMath::InterpEaseOut(0.0f, 1.0f, T, 2.0f));

		if (T >= 1.0f)
		{
			// 完全に消えたら、ポーズを解除して、画面から外し、「消えた」ことを通知する
			// 押されたボタンに合わせたレベル移動は、画面から外す前に頼んでおく
			// (OpenLevel は、その場ではなく、フレームの終わりに移動する)
			ReleasePause();
			RestoreInputMode();   // 入力を、ゲーム操作に戻す(OnClosed を受け取る側が、また切り替えられるよう、先に戻す)
			ExecutePendingAction();
			RemoveFromParent();
			OnClosed.Broadcast();
		}
	}
}

// 受け取ったデータを、画面に反映する(画面に出す前に呼ぶ)
//  画像が出せるものは画像で、出せないものは(あれば)文字で出す
void URhythmResultWidget::SetResult(const FRhythmResultData& Data)
{
	// ボタンが無いときは何もしない(NativeConstruct と同じ理由)
	if (!HasAllParts())
	{
		return;
	}

	// ---- 曲名・難易度 ----
	if (SongTitleText)
	{
		SongTitleText->SetText(Data.SongTitle);
	}
	ShowImageOrText(SongTitleImage, Data.SongTitleTexture, SongTitleText, true);

	const bool bHard = (Data.Difficulty == ESongDifficulty::Hard);
	if (DifficultyText)
	{
		DifficultyText->SetText(bHard ? FText::FromString(TEXT("Hard")) : FText::FromString(TEXT("Normal")));
	}
	ShowImageOrText(DifficultyImage, bHard ? HardDifficultyTexture.Get() : NormalDifficultyTexture.Get(), DifficultyText, true);

	// ---- スコア ----
	// 小数の誤差を避けるため、100倍して整数(例: 100.00 は 10000)にしてから分ける
	const float Score = FMath::Clamp(Data.Score, 0.0f, 100.0f);
	const int32 Hundredths = FMath::RoundToInt(Score * 100.0f);
	ShowNumber(ScoreIntBox, ScoreIntText, FString::Printf(TEXT("%d"), Hundredths / 100), ScoreDigitSize,
		FString::Printf(TEXT("%d"), Hundredths / 100));
	ShowNumber(ScoreDecimalBox, ScoreDecimalText, FString::Printf(TEXT(".%02d"), Hundredths % 100), DecimalDigitSize,
		FString::Printf(TEXT(".%02d"), Hundredths % 100));

	// ---- ランク(スコアから決める / Data.Rank をそのまま使う)----
	const ERhythmRank Rank = bAutoRankFromScore ? ScoreToRank(Score) : Data.Rank;
	if (RankText)
	{
		RankText->SetText(FText::FromString(RankToString(Rank)));
	}
	UTexture2D* RankTexture = nullptr;
	if (const TObjectPtr<UTexture2D>* Found = RankTextures.Find(Rank))
	{
		RankTexture = Found->Get();
	}
	ShowImageOrText(RankImage, RankTexture, RankText, true);

	// ---- 最大コンボと、各判定の数(マイナスの値が来ても 0 として扱う)----
	const FString MaxComboStr = FString::Printf(TEXT("%04d"), FMath::Max(0, Data.MaxCombo));
	ShowNumber(MaxComboBox, MaxComboText, MaxComboStr, SmallDigitSize, FString::Printf(TEXT("MAX COMBO :%s"), *MaxComboStr));

	auto ShowCount = [this](UHorizontalBox* Box, UTextBlock* Text, int32 Value)
		{
			const FString S = FString::Printf(TEXT("%04d"), FMath::Max(0, Value));
			ShowNumber(Box, Text, S, SmallDigitSize, FString::Printf(TEXT(":%s"), *S));
		};
	ShowCount(PerfectBox, PerfectCountText, Data.PerfectCount);
	ShowCount(GreatBox, GreatCountText, Data.GreatCount);
	ShowCount(GoodBox, GoodCountText, Data.GoodCount);
	ShowCount(BadBox, BadCountText, Data.BadCount);
	ShowCount(MissBox, MissCountText, Data.MissCount);

	// ---- ALL PERFECT / FULL COMBO ----
	// 両方 true のときは ALL PERFECT を優先して、どちらか一方だけ出す
	// どちらでもないときは、場所を取らないように Collapsed(非表示)にする
	if (Data.bAllPerfect)
	{
		if (ClearBadgeText) { ClearBadgeText->SetText(FText::FromString(TEXT("ALL PERFECT"))); }
		ShowImageOrText(ClearBadgeImage, AllPerfectTexture, ClearBadgeText, true);
	}
	else if (Data.bFullCombo)
	{
		if (ClearBadgeText) { ClearBadgeText->SetText(FText::FromString(TEXT("FULL COMBO"))); }
		ShowImageOrText(ClearBadgeImage, FullComboTexture, ClearBadgeText, true);
	}
	else
	{
		ShowImageOrText(ClearBadgeImage, nullptr, ClearBadgeText, false);
	}

	// ---- NEW RECORD(記録を更新したときだけ表示)----
	if (NewRecordText)
	{
		NewRecordText->SetText(FText::FromString(TEXT("NEW RECORD")));
	}
	ShowImageOrText(NewRecordImage, NewRecordTexture, NewRecordText, Data.bNewRecord);

	// ---- FAST / LATE(判定より早かった回数 / 遅かった回数)----
	// 数字の画像が両方出せたら、まとめの文字は隠す。出せなければ、まとめの文字を出す
	const int32 Fast = FMath::Max(0, Data.FastCount);
	const int32 Late = FMath::Max(0, Data.LateCount);
	const bool bFastImg = ShowNumber(FastBox, nullptr, FString::Printf(TEXT("%d"), Fast), SmallDigitSize, FString());
	const bool bLateImg = ShowNumber(LateBox, nullptr, FString::Printf(TEXT("%d"), Late), SmallDigitSize, FString());
	if (FastLateText)
	{
		if (bFastImg && bLateImg)
		{
			FastLateText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			FastLateText->SetText(FText::FromString(FString::Printf(TEXT("FAST %d  /  LATE %d"), Fast, Late)));
			FastLateText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

// 数字の文字列を、数字の画像で並べる
bool URhythmResultWidget::ShowNumber(UHorizontalBox* Box, UTextBlock* Text, const FString& Digits,
	const FVector2D& Size, const FString& FallbackText)
{
	// 文字ごとの画像を集める(1つでも足りなければ、画像では出さない)
	TArray<UTexture2D*> Textures;
	bool bReady = (Box != nullptr);
	if (bReady)
	{
		for (const TCHAR Ch : Digits)
		{
			UTexture2D* Tex = nullptr;
			if (Ch >= TEXT('0') && Ch <= TEXT('9'))
			{
				const int32 Index = Ch - TEXT('0');
				if (DigitTextures.IsValidIndex(Index)) { Tex = DigitTextures[Index]; }
			}
			else if (Ch == TEXT('.'))
			{
				Tex = DotTexture;
			}

			if (!Tex)
			{
				bReady = false;
				break;
			}
			Textures.Add(Tex);
		}
	}

	if (!bReady)
	{
		// 画像で出せないとき: 入れ物は隠して、文字(あれば)を出す
		if (Box) { Box->SetVisibility(ESlateVisibility::Collapsed); }
		if (Text)
		{
			Text->SetText(FText::FromString(FallbackText));
			Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		return false;
	}

	// 並べ直す(前の数字を消してから、新しく作る)
	Box->ClearChildren();
	for (int32 i = 0; i < Textures.Num(); ++i)
	{
		UImage* DigitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		DigitImage->SetBrushFromTexture(Textures[i], false);
		// 小数点は、数字の半分の幅にする
		const bool bDot = (Digits[i] == TEXT('.'));
		DigitImage->SetDesiredSizeOverride(bDot ? FVector2D(Size.X * 0.5, Size.Y) : Size);
		Box->AddChildToHorizontalBox(DigitImage);
	}
	Box->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Text) { Text->SetVisibility(ESlateVisibility::Collapsed); }
	return true;
}

// フェードアウトを始める。終わると自動で画面から消える
void URhythmResultWidget::CloseWithFade()
{
	// すでに消え始めているときは、二重に始めない
	if (State == EState::FadingOut)
	{
		return;
	}

	State = EState::FadingOut;
	ElapsedFade = 0.0f;

	// 消えている最中に、ボタンを連打されても反応しないようにする(見た目は変えない)
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

// ---- ボタンが押されたとき ----
// どちらも「通知を出す → フェードアウト」だけ。
// 実際の処理(やり直す・タイトルに戻る)は、デリゲートに繋いだ担当者が行う

// ゲームパッドの決定を ○ にする
//  DS4Windows 経由では、○ は Xbox の B(Gamepad_FaceButton_Right)、× は A(Gamepad_FaceButton_Bottom)として届く
//  UMG の標準は A(×)で決定なので、ボタンに届く前に受け取って、○ で押し、× は無効にする
FReply URhythmResultWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// 選択の移動: 十字キー・左スティックの左右/上下、キーボードの矢印
	if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left
		|| Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up
		|| Key == EKeys::Left || Key == EKeys::Up)
	{
		MoveSelection(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right
		|| Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down
		|| Key == EKeys::Right || Key == EKeys::Down)
	{
		MoveSelection(+1);
		return FReply::Handled();
	}

	if (Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Enter)   // ○ / Enter
	{
		if (State == EState::Shown && !InKeyEvent.IsRepeat() && RetryButton && TitleButton)
		{
			if (SelectedIndex == 0)
			{
				HandleRetry();
			}
			else
			{
				HandleTitle();
			}
		}
		return FReply::Handled();
	}

	if (Key == EKeys::Gamepad_FaceButton_Bottom)   // ×(決定には使わない)
	{
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void URhythmResultWidget::HandleRetry()
{
	if (State == EState::FadingOut)
	{
		return;   // 消えている途中の連打は無視
	}
	PendingAction = EPendingAction::Retry;   // フェードアウトが終わったあとに、やり直す
	OnRetryRequested.Broadcast();            // 「リトライが押された」を知らせる
	CloseWithFade();
}

void URhythmResultWidget::HandleTitle()
{
	if (State == EState::FadingOut)
	{
		return;
	}
	PendingAction = EPendingAction::Title;   // フェードアウトが終わったあとに、曲選択へ戻る
	OnTitleRequested.Broadcast();            // 「タイトルが押された」を知らせる
	CloseWithFade();
}

// フェードアウトが終わったあとに、押されたボタンに合わせてレベルを移動する
//  ・タイトル : SongSelectLevelName のレベルを開く(曲選択画面に戻る)
//  ・リトライ : いまのレベルを開き直す(曲を最初からやり直す)
// どちらも、WBP のクラスデフォルトの設定が、空 / オフのときは、何もしない
void URhythmResultWidget::ExecutePendingAction()
{
	const EPendingAction Action = PendingAction;
	PendingAction = EPendingAction::None;   // 二重に実行しないよう、先に消しておく

	UWorld* World = GetWorld();
	if (!World || Action == EPendingAction::None)
	{
		return;
	}

	switch (Action)
	{
	case EPendingAction::Title:
		if (SongSelectLevelName.IsNone())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("RhythmResultWidget: SongSelectLevelName is empty, so the level is not changed (set it in WBP_Result class defaults, or handle OnTitleRequested)."));
			return;
		}
		UGameplayStatics::OpenLevel(World, SongSelectLevelName);
		break;

	case EPendingAction::Retry:
	{
		if (!bReloadLevelOnRetry)
		{
			return;
		}
		// いまのレベルの名前(エディタでプレイ中の「UEDPIE_0_」は、取り除いてもらう)
		// 開き直したあとは、曲選択を出さずに、同じ曲で始めてもらう
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UGameSessionSubsystem* Session = GI->GetSubsystem<UGameSessionSubsystem>())
			{
				Session->RequestSkipSongSelect();
			}
		}

		const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(World, /*bRemovePrefixString=*/ true);
		UGameplayStatics::OpenLevel(World, FName(*CurrentLevel));
		break;
	}

	default:
		break;
	}
}