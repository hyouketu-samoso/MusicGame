// Fill out your copyright notice in the Description page of Project Settings.

// ============================================================
// RhythmResultWidget.cpp
//  リザルト画面の処理。
//  - SetResult() で受け取ったデータを、WBP側の文字に流し込む
//  - リトライ / タイトルのボタンは「押された」通知を出すだけ(レベル遷移はしない)
//  - 表示するときにフェードイン、消すときにフェードアウトする
//  - 表示している間はゲームを止め(ポーズ)、消えるときに解除する
// ============================================================
#include "RhythmResultWidget.h"
#include "Components/Button.h"        // UButton を使うために必要
#include "Components/TextBlock.h"     // UTextBlock を使うために必要
#include "Kismet/GameplayStatics.h"  // SetGamePaused を使うために必要

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

	// キーボードやパッドで操作できるように、最初のフォーカスをリトライに置く
	RetryButton->SetKeyboardFocus();

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

// 毎フレーム呼ばれる。フェードの進み具合をここで更新する
void URhythmResultWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

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
			ReleasePause();
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

void URhythmResultWidget::HandleRetry()
{
	if (State == EState::FadingOut)
	{
		return;   // 消えている途中の連打は無視
	}
	OnRetryRequested.Broadcast();   // 「リトライが押された」を知らせる
	CloseWithFade();
}

void URhythmResultWidget::HandleTitle()
{
	if (State == EState::FadingOut)
	{
		return;
	}
	OnTitleRequested.Broadcast();   // 「タイトルが押された」を知らせる
	CloseWithFade();
}