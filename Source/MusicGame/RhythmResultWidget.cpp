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
#include "Components/TextBlock.h"     // UTextBlock を使うために必要
#include "Kismet/GameplayStatics.h"  // SetGamePaused を使うために必要
#include "GameFramework/PlayerController.h"   // 入力モードの切り替え(FInputModeUIOnly)に必要
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"   // EKeys(ゲームパッドのボタン名)
#include "GameSessionSubsystem.h"

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

	// 判定の数を「:0012」のように4桁ゼロ埋めの文字にする
	// マイナスの値が来ても 0 として扱う
	FText Count4(int32 Value)
	{
		return FText::FromString(FString::Printf(TEXT(":%04d"), FMath::Max(0, Value)));
	}
}

// WBP側に必要な部品が全部あるかを調べる
// 1つでも無い(NULL)と、使った瞬間にクラッシュするので、その前にここで止める
bool URhythmResultWidget::HasAllParts() const
{
	return SongTitleText && DifficultyText && RankText && ScoreIntText && ScoreDecimalText
		&& MaxComboText && PerfectCountText && GreatCountText && GoodCountText
		&& BadCountText && MissCountText && ClearBadgeText && NewRecordText
		&& FastLateText && RetryButton && TitleButton;
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

// ゲームパッド操作の準備
//  ・入力を、UI 操作に切り替える(ゲーム操作のままだと、十字キーや×ボタンが、ボタンに届かない)
//  ・リトライのボタンに、フォーカスを置く
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
	Mode.SetWidgetToFocus(RetryButton->TakeWidget());
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = true;   // マウスでも押せるようにする
	bInputModeChanged = true;

	RetryButton->SetUserFocus(PC);
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
// 十字キー・左スティックの移動と、×ボタンの決定は、UMG の標準の動きに任せている
void URhythmResultWidget::UpdateFocusVisual()
{
	if (!RetryButton || !TitleButton)
	{
		return;
	}

	const bool bRetryFocus = RetryButton->HasAnyUserFocus();
	const bool bTitleFocus = TitleButton->HasAnyUserFocus();
	const int32 FocusIndex = bRetryFocus ? 0 : (bTitleFocus ? 1 : -1);

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

	// どちらにもフォーカスが無いとき(画面の何も無い所を、クリックしたときなど)は、リトライに戻す
	if (FocusIndex == -1 && State == EState::Shown)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			RetryButton->SetUserFocus(PC);
		}
	}
}

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

// 受け取ったデータを、画面の文字に反映する(画面に出す前に呼ぶ)
void URhythmResultWidget::SetResult(const FRhythmResultData& Data)
{
	// 部品が無いときは何もしない(NativeConstruct と同じ理由)
	if (!HasAllParts())
	{
		return;
	}

	// ---- 曲名・難易度 ----
	SongTitleText->SetText(Data.SongTitle);
	DifficultyText->SetText(Data.Difficulty == ESongDifficulty::Hard
		? FText::FromString(TEXT("Hard"))
		: FText::FromString(TEXT("Normal")));

	// ---- ランク ----
	RankText->SetText(FText::FromString(RankToString(Data.Rank)));

	// ---- スコア ----
	// 「100」と「.00」を別々の文字にして、小数部を小さく見せられるようにする
	// 小数の誤差を避けるため、100倍して整数(例: 100.00 は 10000)にしてから分ける
	const float Score = FMath::Clamp(Data.Score, 0.0f, 100.0f);
	const int32 Hundredths = FMath::RoundToInt(Score * 100.0f);
	ScoreIntText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Hundredths / 100)));        // 整数部
	ScoreDecimalText->SetText(FText::FromString(FString::Printf(TEXT(".%02d"), Hundredths % 100))); // 小数部

	// ---- 最大コンボと、各判定の数 ----
	MaxComboText->SetText(FText::FromString(
		FString::Printf(TEXT("MAX COMBO :%04d"), FMath::Max(0, Data.MaxCombo))));

	PerfectCountText->SetText(Count4(Data.PerfectCount));
	GreatCountText->SetText(Count4(Data.GreatCount));
	GoodCountText->SetText(Count4(Data.GoodCount));
	BadCountText->SetText(Count4(Data.BadCount));
	MissCountText->SetText(Count4(Data.MissCount));

	// ---- ALL PERFECT / FULL COMBO ----
	// 両方 true のときは ALL PERFECT を優先して、どちらか一方だけ出す
	// どちらでもないときは、場所を取らないように Collapsed(非表示)にする
	if (Data.bAllPerfect)
	{
		ClearBadgeText->SetText(FText::FromString(TEXT("ALL PERFECT")));
		ClearBadgeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (Data.bFullCombo)
	{
		ClearBadgeText->SetText(FText::FromString(TEXT("FULL COMBO")));
		ClearBadgeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		ClearBadgeText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// ---- NEW RECORD(記録を更新したときだけ表示)----
	NewRecordText->SetText(FText::FromString(TEXT("NEW RECORD")));
	NewRecordText->SetVisibility(Data.bNewRecord
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);

	// ---- FAST / LATE(判定より早かった回数 / 遅かった回数)----
	FastLateText->SetText(FText::FromString(
		FString::Printf(TEXT("FAST %d  /  LATE %d"),
			FMath::Max(0, Data.FastCount), FMath::Max(0, Data.LateCount))));
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

	if (Key == EKeys::Gamepad_FaceButton_Right)   // ○
	{
		if (State == EState::Shown && !InKeyEvent.IsRepeat() && RetryButton && TitleButton)
		{
			if (RetryButton->HasAnyUserFocus())
			{
				HandleRetry();
			}
			else if (TitleButton->HasAnyUserFocus())
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