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
//  素材 : ランク / ALL PERFECT・FULL COMBO / NEW RECORD は、画像でも表示できる
//         (画像の部品と画像が両方そろっていれば画像、そろっていなければ、いままでの文字を出す)
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
class UImage;
class UHorizontalBox;
class UTexture2D;

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
	// 曲名の画像(曲ごとに用意する。未設定なら、曲名の文字を出す)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UTexture2D> SongTitleTexture = nullptr;
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

	// ---- ランクの決め方 ----
	// true のとき、渡された Rank は使わずに、スコア(0.00 から 100.00)から、ランクを決める
	// false のとき、ゲーム側が渡した Rank をそのまま使う
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Rank")
	bool bAutoRankFromScore = true;

	// 各ランクになる、スコアの下限(例: SS は 95.00 以上)。これ未満はすべて D
	// 値を変えるときは、SS > S > A > B > C の順に、小さくなるようにする
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Rank", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float RankThresholdSS = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Rank", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float RankThresholdS = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Rank", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float RankThresholdA = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Rank", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float RankThresholdB = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Rank", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float RankThresholdC = 60.0f;

	// ---- 画像の素材(WBP のクラスデフォルトで設定する。デザイナーの素材が来たら、差し替える) ----
	// 未設定のものは、画像を出さずに、文字を出す
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TMap<ERhythmRank, TObjectPtr<UTexture2D>> RankTextures;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> AllPerfectTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> FullComboTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> NewRecordTexture;

	// ---- 画像を出す部品(WBP に Image を置いて、同じ名前にする。無くても動く) ----
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> RankImage;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ClearBadgeImage;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> NewRecordImage;

	// ---- 数字の画像(0 から 9 の10枚と、小数点)----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Numbers")
	TArray<TObjectPtr<UTexture2D>> DigitTextures;   // 0 から 9 の順に10枚

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Numbers")
	TObjectPtr<UTexture2D> DotTexture;

	// 数字1つの大きさ(スコア / ほかの数字)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Numbers")
	FVector2D ScoreDigitSize = FVector2D(64.0, 96.0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Numbers")
	FVector2D DecimalDigitSize = FVector2D(40.0, 60.0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Numbers")
	FVector2D SmallDigitSize = FVector2D(32.0, 48.0);

	// 難易度の画像
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> NormalDifficultyTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> HardDifficultyTexture;

	// ---- 数字を並べる入れ物(WBP に Horizontal Box を置いて、同じ名前にする。無くても動く)----
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> ScoreIntBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> ScoreDecimalBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> MaxComboBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> PerfectBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> GreatBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> GoodBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> BadBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> MissBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> FastBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> LateBox;

	// ---- 画像を出す部品(曲名・難易度)----
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> SongTitleImage;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> DifficultyImage;

	// ---- 文字の部品(もう使わないなら、WBP から消してよい。画像が出せないときの代わりに出る)----
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SongTitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DifficultyText;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> RankText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ScoreIntText;       // 「100」
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ScoreDecimalText;   // 「.00」

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MaxComboText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PerfectCountText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GreatCountText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GoodCountText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> BadCountText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissCountText;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ClearBadgeText;   // ALL PERFECT / FULL COMBO
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> NewRecordText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FastLateText;

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

	// スコア(0.00 から 100.00)から、ランクを決める
	ERhythmRank ScoreToRank(float Score) const;

	// 画像と文字の切り替え
	//  bShow が false: 両方隠す / 画像の部品と画像がある: 画像を出して文字を隠す / ない: 文字を出す
	void ShowImageOrText(UImage* Image, UTexture2D* Texture, UTextBlock* Text, bool bShow);

	// 数字の文字列(例: "0123"、".45")を、数字の画像で並べる
	//  画像が出せたら true(Text は隠す)/ 出せないとき(入れ物か画像が無い)は false(Text に Fallback を出す)
	bool ShowNumber(UHorizontalBox* Box, UTextBlock* Text, const FString& Digits, const FVector2D& Size, const FString& FallbackText);

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