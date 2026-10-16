// Fill out your copyright notice in the Description page of Project Settings.

// ============================================================
// RhythmResultWidget
//  役割 : リザルト画面の骨組み
//         (曲名・難易度 / ランク / スコア / 判定の数 / 最大コンボ /
//          ALL PERFECT・FULL COMBO / NEW RECORD / FAST・LATE / リトライ・タイトル)
//  方針 : 表示するデータは FRhythmResultData にまとめて渡す
//         リトライ・タイトルは、「押された」ことを通知して、フェードアウトのあとにレベルを移動する
//         (移動先は SongSelectLevelName。空のときは、移動せず、通知だけ。
//          同じレベルの中で曲選択を出すときは、OnTitleRequested / OnClosed に繋ぐ)
//  演出 : 表示するときにフェードイン、ボタンを押すとフェードアウトして消える
//  時間 : 表示している間はゲームを止める(ポーズ)。消えるときに解除する
//  操作 : マウス・キーボード・ゲームパッドで、リトライ / タイトルを選べる
//         (十字キー・左スティックで移動、○ボタン(Xbox の B)で決定。×ボタンでは決定しない)
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SongSelectWidget.h"   // ESongDifficulty を使う
#include "RhythmResultWidget.generated.h"

class UButton;
class UTextBlock;

UENUM(BlueprintType)
enum class ERhythmRank : uint8
{
	SS,
	S,
	A,
	B,
	C,
	D
};

// リザルト画面に表示するデータ(ゲーム側で集計して、まとめて渡す)
USTRUCT(BlueprintType)
struct FRhythmResultData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText SongTitle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ESongDifficulty Difficulty = ESongDifficulty::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ERhythmRank Rank = ERhythmRank::D;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Score = 0.0f;   // 0.00 から 100.00

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxCombo = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PerfectCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 GreatCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 GoodCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BadCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MissCount = 0;

	// 何を ALL PERFECT / FULL COMBO とするかは、ゲーム側(集計する人)が決めて渡す
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllPerfect = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bFullCombo = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bNewRecord = false;

	// 判定よりも早かった回数 / 遅かった回数
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 FastCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LateCount = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResultRetryRequested);   // リトライが押された
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResultTitleRequested);   // タイトルが押された
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResultClosed);           // フェードが終わって、消えた後

UCLASS()
class MUSICGAME_API URhythmResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- 通知(処理の担当者はここに繋ぐ) ----
	UPROPERTY(BlueprintAssignable, Category = "Result")
	FOnResultRetryRequested OnRetryRequested;

	UPROPERTY(BlueprintAssignable, Category = "Result")
	FOnResultTitleRequested OnTitleRequested;

	UPROPERTY(BlueprintAssignable, Category = "Result")
	FOnResultClosed OnClosed;

	// ---- 表示するデータを渡す(画面に出す前に呼ぶ) ----
	UFUNCTION(BlueprintCallable, Category = "Result")
	void SetResult(const FRhythmResultData& Data);

	// フェードアウトして消す(リトライ・タイトルからも呼ばれる)
	UFUNCTION(BlueprintCallable, Category = "Result")
	void CloseWithFade();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ゲームパッドの決定ボタンを、○にする(ボタンに届く前に受け取る)
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// ---- 演出の調整値 ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Fade")
	float FadeInDuration = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Fade")
	float FadeOutDuration = 0.4f;

	// 表示している間、ゲームを止める(ポーズする)かどうか
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	bool bPauseGameWhileShown = true;

	// ---- ボタンを押したあとの移動(フェードアウトが終わってから行う) ----

	// 「タイトル」ボタンで移動するレベルの名前(曲選択画面があるレベル。例: LVL_SongSelect)
	// 空のままにすると、レベルは移動しない(通知だけ出す。同じレベルの中で曲選択を出すときは、こちら)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Flow")
	FName SongSelectLevelName;

	// 「リトライ」ボタンで、いまのレベルを開き直す(曲を最初からやり直す)かどうか
	// false のときは、レベルは移動しない(通知だけ出す)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Flow")
	bool bReloadLevelOnRetry = true;

	// ---- ゲームパッド / キーボードで、選んでいるボタンの見た目 ----
	// (WBP のクラスデフォルトで変えられる)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Focus")
	float FocusedScale = 1.1f;   // 選んでいるボタンの拡大率

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Focus")
	FLinearColor FocusedColor = FLinearColor(1.0f, 0.85f, 0.3f, 1.0f);   // 選んでいるボタンの色

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Focus")
	FLinearColor NormalColor = FLinearColor::White;   // 選んでいないボタンの色

	// ---- WBP側に、同じ名前の部品を置く(「変数である」にチェック) ----
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SongTitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> DifficultyText;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> RankText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ScoreIntText;       // 「100」
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ScoreDecimalText;   // 「.00」

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> MaxComboText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> PerfectCountText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> GreatCountText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> GoodCountText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> BadCountText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> MissCountText;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ClearBadgeText;   // ALL PERFECT / FULL COMBO
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> NewRecordText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> FastLateText;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> RetryButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> TitleButton;

private:
	// ボタンのクリック(AddDynamic には UFUNCTION が必要)
	UFUNCTION() void HandleRetry();
	UFUNCTION() void HandleTitle();

	// 部品が1つでも足りないと false(足りないときは、クラッシュさせずにログを出す)
	bool HasAllParts() const;

	// このウィジェットがかけたポーズだけを解除する
	void ReleasePause();

	// フェードアウトが終わったあとに、押されたボタンに合わせてレベルを移動する
	void ExecutePendingAction();

	// ゲームパッド操作の準備(UI 操作に切り替えて、リトライにフォーカスを置く)
	void SetupGamepadFocus();

	// 選んでいるボタンの見た目を、フォーカスに合わせて更新する
	void UpdateFocusVisual();

	// このウィジェットが変えた入力モードを、ゲーム操作に戻す
	void RestoreInputMode();

	bool bFocusSetup = false;          // ゲームパッド操作の準備をしたか
	bool bInputModeChanged = false;    // 入力モードを変えたか(戻すときの目印)
	bool bPrevShowCursor = false;      // 変える前の、マウスカーソルの表示
	int32 LastFocusIndex = -2;         // 前回のフォーカス(0=リトライ / 1=タイトル / -1=なし / -2=未設定)

	// どのボタンが押されたか(フェードアウトが終わるまで覚えておく)
	enum class EPendingAction : uint8
	{
		None,
		Retry,
		Title
	};

	EPendingAction PendingAction = EPendingAction::None;

	enum class EState : uint8
	{
		FadingIn,
		Shown,
		FadingOut
	};

	EState State = EState::FadingIn;
	float ElapsedFade = 0.0f;

	// このウィジェットがポーズをかけたか(自分がかけた分だけ解除するため)
	bool bPausedByThis = false;
};