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
//  順番表示 : 土台 → 難易度 → 判定(1項目ずつ) → MAX COMBO → FAST / LATE → SCORE(スロットのように回って、左の桁から止まる)
//             → RANK → FULL COMBO(達成時のみ) → NEW RECORD(達成時のみ) → ボタン、の順に出す
//             ボタンが出るまでは、操作できない。効果音は、クラス既定値に入れるだけで鳴る(空なら鳴らさない)
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
class UWidget;
class USoundBase;
class UAudioComponent;

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

	// ---- 順番表示の調整値(秒) ----
	// 土台が出てから、最初の項目(難易度)が出るまで
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence", meta = (ClampMin = "0.0"))
	float StartDelay = 0.4f;

	// 段階と段階の間(難易度 → 判定 → MAX COMBO → FAST/LATE → SCORE → RANK → ...)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence", meta = (ClampMin = "0.0"))
	float StepInterval = 0.5f;

	// RANK が出てから、FULL COMBO(ALL PERFECT)が出るまで(ゆっくり出したいときは、大きくする)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence", meta = (ClampMin = "0.0"))
	float BadgeDelay = 1.0f;

	// FULL COMBO が出てから、NEW RECORD が出るまで
	// (FULL COMBO が無いときは、RANK が出てから、NEW RECORD が出るまで)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence", meta = (ClampMin = "0.0"))
	float NewRecordDelay = 1.0f;

	// 判定一覧の1項目ごとの間(PERFECT → GREAT → ... / FAST → LATE)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence", meta = (ClampMin = "0.0"))
	float JudgeItemInterval = 0.2f;

	// 1つの項目が、透明から見えるまで
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence", meta = (ClampMin = "0.01"))
	float RevealFadeDuration = 0.25f;

	// 項目が出るときの、すべり込みの距離(ピクセル。0 にすると、その場でふわっと出るだけ)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence")
	FVector2D RevealSlide = FVector2D(0.0, 14.0);

	// SCORE がスロットのように回り始めてから、最後の桁が止まるまでの時間
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence", meta = (ClampMin = "0.1"))
	float ScoreCountDuration = 2.0f;

	// 回っている数字が、次の数字に変わる間隔(秒。小さいほど速く回る)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence", meta = (ClampMin = "0.01"))
	float SlotChangeInterval = 0.05f;

	// 桁が、左から1つずつ止まっていく間隔(秒。0 にすると、全部の桁が同時に止まる)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence", meta = (ClampMin = "0.0"))
	float SlotStopInterval = 0.25f;

	// true: スコアを「005.00」のように、0 を付けて3桁で出す(ゲーム中の表示と同じ)
	// false: 止まったあとは「5.00」のように、0 を付けずに出す
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Sequence")
	bool bScoreLeadingZeros = true;

	// 桁が1つ止まるたびに鳴る音(最後の桁は、ScoreFixSE が鳴る。空なら鳴らさない)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> ScoreDigitStopSE;

	// ---- 効果音(あとで、クラス既定値に入れるだけ。空なら鳴らさない) ----
	// 土台が出た瞬間(1回)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> OpenSE;

	// 判定の数(PERFECT / GREAT / GOOD / BAD / MISS)が1項目出るたびに(1項目ごとに1回)
	// ※ プレイ中の判定SEは使い回さない(別の音を用意する)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> JudgeItemSE;

	// MAX COMBO が出たとき(1回)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> MaxComboSE;

	// FAST / LATE が1項目出るたびに(1項目ごとに1回)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> FastLateSE;

	// SCORE が増えている間(ループする音を設定する。止まるとき自動で止める)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> ScoreCountSE;

	// SCORE が最終値で止まったとき(1回)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> ScoreFixSE;

	// ランクが出たとき(1回。全ランク共通)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> RankSE;

	// FULL COMBO / ALL PERFECT のバッジが出たとき(1回)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> FullComboSE;

	// NEW RECORD のバッジが出たとき(1回)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> NewRecordSE;

	// ボタンの選択を動かしたとき(SE_UI_Move)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> MoveSE;

	// ボタンを決定したとき(SE_UI_Decide)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|SE")
	TObjectPtr<USoundBase> DecideSE;

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

	// 判定の数・MAX COMBO を、右詰めにする桁数(FAST / LATE は、右詰めにしない)
	// (例: 4 なら、4桁ぶんの幅を空けて、右にそろえる。0 にすると、右詰めにしない = 左詰め)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Numbers", meta = (ClampMin = "0"))
	int32 CountRightAlignDigits = 4;

	// 難易度の画像(5つ。曲選択の難易度と同じ。未設定の難易度は、文字を出す)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> VeryEasyDifficultyTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> EasyDifficultyTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> NormalDifficultyTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> HardDifficultyTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result|Images")
	TObjectPtr<UTexture2D> VeryHardDifficultyTexture;

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

	// ---- 順番表示の「ひとまとまり」(WBP で、ラベルと数字を Overlay などでまとめて、同じ名前にする) ----
	// 置かなくても動く(その場合は、数字・画像の部品だけが、順番に出る。ラベルは最初から見える)
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> PlayInfoGroup;   // 難易度(と曲名)
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> PerfectGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> GreatGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> GoodGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> BadGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> MissGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> MaxComboGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> FastGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> LateGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ScoreGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> RankGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> BadgeGroup;       // FULL COMBO / ALL PERFECT
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> NewRecordGroup;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ButtonsGroup;     // ボタンの中身(枠は含めない)

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> RetryButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> TitleButton;

private:
	// ボタンのクリック(AddDynamic には UFUNCTION が必要)
	UFUNCTION() void HandleRetry();
	UFUNCTION() void HandleTitle();

	// マウスを乗せたとき(選択を、そのボタンに移す)
	UFUNCTION() void HandleRetryHovered();
	UFUNCTION() void HandleTitleHovered();

	// 選択を動かす(Direction: -1 = 前 / +1 = 次。端で止まる)
	void MoveSelection(int32 Direction);

	// 部品が1つでも足りないと false(足りないときは、クラッシュさせずにログを出す)
	bool HasAllParts() const;

	// このウィジェットがかけたポーズだけを解除する
	void ReleasePause();

	// フェードアウトが終わったあとに、押されたボタンに合わせてレベルを移動する
	void ExecutePendingAction();

	// スコア(0.00 から 100.00)から、ランクを決める
	ERhythmRank ScoreToRank(float Score) const;

	// 難易度に合った画像を返す(未設定なら nullptr)
	UTexture2D* GetDifficultyTexture(ESongDifficulty Difficulty) const;

	// 画像と文字の切り替え
	//  bShow が false: 両方隠す / 画像の部品と画像がある: 画像を出して文字を隠す / ない: 文字を出す
	void ShowImageOrText(UImage* Image, UTexture2D* Texture, UTextBlock* Text, bool bShow);

	// 数字の文字列(例: "0123"、".45")を、数字の画像で並べる
	//  画像が出せたら true(Text は隠す)/ 出せないとき(入れ物か画像が無い)は false(Text に Fallback を出す)
	//  PadToDigits が 1 以上のとき、桁が足りない分だけ、左に空白を入れる(右詰めになる)
	bool ShowNumber(UHorizontalBox* Box, UTextBlock* Text, const FString& Digits, const FVector2D& Size, const FString& FallbackText, int32 PadToDigits = 0);

	// ---- 順番表示 ----
	// スコア(100倍した整数)を、画像の数字(または文字)で出す
	void ShowScoreHundredths(int32 Hundredths);

	// 5桁の数字の文字列(例: "01234")を、「012」「.34」に分けて出す(スロットの回転中にも使う)
	void ShowScoreDigits(const FString& FiveDigits);

	// 各段階の、出す部品の一覧を作る(ひとまとまりがあればそれ、無ければ数字や画像の部品)
	void BuildTargetTables();

	// 順番表示の前に、すべての部品を、見えない状態にする
	void HideAllForSequence();

	// 順番表示の予定表を作る(達成していない段階は、飛ばす)
	void BuildSequence();

	// 順番表示を進める(Revealing の間、毎フレーム)
	void TickSequence(float DeltaTime);

	// 部品を、見えるようにし始める
	void StartReveal(UWidget* Target);

	// 出ている最中の部品の、透明度と位置を進める
	void UpdateActiveReveals();

	// 効果音を鳴らす(空なら何もしない。ポーズ中も鳴る)
	void PlaySE(USoundBase* Sound);

	// スコアが増えている間のループ音を止める
	void StopCountAudio();

	// ゲームパッド操作の準備(UI 操作に切り替える。選択はコードで管理するので、ボタンにはフォーカスしない)
	void SetupGamepadFocus();

	// 選んでいるボタンの見た目を、SelectedIndex に合わせて更新する
	void UpdateFocusVisual();

	// このウィジェットが変えた入力モードを、ゲーム操作に戻す
	void RestoreInputMode();

	bool bFocusSetup = false;          // ゲームパッド操作の準備をしたか
	bool bInputModeChanged = false;    // 入力モードを変えたか(戻すときの目印)
	bool bPrevShowCursor = false;      // 変える前の、マウスカーソルの表示
	int32 LastFocusIndex = -2;         // 前回、見た目に反映した選択(-2 = 未設定)
	int32 SelectedIndex = 0;           // いま選んでいるボタン(0=リトライ / 1=タイトル)

	// 順番表示の1段階分の予定
	enum class EStepKind : uint8
	{
		Reveal,       // 部品を出す(効果音つき)
		ScoreStart,   // SCORE を出して、数え始める
		ScoreEnd,     // 数え終わる(最終値で止める)
		Buttons       // ボタンを出して、操作できるようにする
	};

	struct FRevealStep
	{
		float Time = 0.0f;
		EStepKind Kind = EStepKind::Reveal;
		TArray<TWeakObjectPtr<UWidget>> Targets;
		USoundBase* SE = nullptr;
	};

	struct FActiveReveal
	{
		TWeakObjectPtr<UWidget> Widget;
		float StartTime = 0.0f;
	};

	// 各段階で出す部品(NativeConstruct で作る)
	TArray<TWeakObjectPtr<UWidget>> PlayInfoTargets, PerfectTargets, GreatTargets, GoodTargets, BadTargets,
		MissTargets, MaxComboTargets, FastTargets, LateTargets, ScoreTargets, RankTargets, BadgeTargets,
		NewRecordTargets, ButtonsTargets;

	TArray<FRevealStep> Steps;
	int32 NextStep = 0;
	TArray<FActiveReveal> ActiveReveals;

	// 各部品の、もともとの Render Transform の移動量(WBP で動かしてある分を、演出で消さないため)
	TMap<TWeakObjectPtr<UWidget>, FVector2D> BaseTranslation;
	float SeqElapsed = 0.0f;           // 順番表示を始めてからの秒数
	bool bButtonsStepDone = false;     // ボタンを出す段階に入ったか

	// スコアの数え上げ
	bool bCounting = false;
	float CountStartTime = 0.0f;
	int32 LastCountShown = -1;
	int32 LastStoppedDigits = 0;       // これまでに止まった桁の数(止まる音を鳴らすため)
	int32 FinalHundredths = 0;         // 最終スコア(100倍した整数)
	bool bBadgeEarned = false;         // FULL COMBO / ALL PERFECT を達成したか
	bool bNewRecordEarned = false;     // 記録を更新したか

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CountAudio;   // 数え上げ中のループ音

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
		FadingIn,     // 土台が見えてくる
		Revealing,    // 項目を順番に出している(操作できない)
		Shown,        // 全部出て、操作できる
		FadingOut
	};

	EState State = EState::FadingIn;
	float ElapsedFade = 0.0f;

	// このウィジェットがポーズをかけたか(自分がかけた分だけ解除するため)
	bool bPausedByThis = false;
};