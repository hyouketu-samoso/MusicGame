// ============================================================
// TimingAdjustWidget
//  役割 : 入力タイミングの調整画面。
//         一定間隔で落ちてくるノーツに合わせてスティックを倒し、
//         拍とのズレ(ms)の平均から入力オフセットを自動で決めて保存する。
//  入力 : 左右スティックの左右4方向（本編と同じレーン番号）
//         Lane 0=左スティック左 / 1=左スティック右 / 2=右スティック左 / 3=右スティック右
//  演出 : 倒したときに SE と、倒した方向のエフェクト画像を出す
//  操作 : 右ボタン(B)=決定（戻るボタン） / 下ボタン(A)・BackSpace=戻る
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimingAdjustWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class USoundBase;
class USoundClass;

// 画面を閉じたときに通知する
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimingAdjustClosed);

UCLASS()
class MUSICGAME_API UTimingAdjustWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTimingAdjustWidget(const FObjectInitializer& ObjectInitializer);

	// 画面を閉じたときに呼ばれる
	UPROPERTY(BlueprintAssignable, Category = "TimingAdjust")
	FOnTimingAdjustClosed OnClosed;

	// 画面を閉じる（戻るボタン・A・BackSpace から呼ばれる）
	UFUNCTION(BlueprintCallable, Category = "TimingAdjust")
	void Close();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;

	// ============================================================
	// ノーツの流れ
	// ============================================================

	// ノーツが判定ラインに届く間隔（秒）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "0.5", Units = "s"))
	float BeatIntervalSec = 1.0f;

	// ノーツが出現してから判定ラインに届くまでの秒数
	// （bUseNoteSpeedSetting が true のときは、開いた時点のノーツ速度設定の到達時間で上書きされる）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "0.2", Units = "s"))
	float NoteTravelSec = 1.0f;

	// true: 曲選択で選んだノーツ速度（到達時間）をこの画面にも使う
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust")
	bool bUseNoteSpeedSetting = true;

	// true: 拍の間隔がノーツの到達時間より短いときは、到達時間まで伸ばす
	// （伸ばさないと、ノーツが途中の位置から急に現れる。速度1〜5の到達時間は 1.2〜2.4 秒）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust")
	bool bExtendBeatToTravel = true;

	// true: 判定の範囲外でもスティックを倒したら、そのノーツを消す
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust")
	bool bConsumeNoteOnInput = true;

	// 判定ラインを通り過ぎたあと、見えたままにする秒数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "0.0", Units = "s"))
	float PassVisibleSec = 0.2f;

	// NoteImage の Render Translation の Y（出現位置 → 判定ライン、ピクセル）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Note")
	float NoteStartY = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Note")
	float NoteEndY = 300.0f;

	// 遠近感の強さ。1 = 画面上を等速で落とす（初期値）。
	// 大きくするほど「奥ではゆっくり、手前に来るほど速い」動きになり、ゲーム内の3Dの見え方に近づく。
	// 到達時間（秒）は変わらない。目安：2〜4 くらいから、ゲーム内と見比べて決める
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Note", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float NoteDepthRatio = 1.0f;

	// ノーツが画面に現れ始める時点（0〜1。0 = 到達時間の最初から / 0.3 = 到達時間の30%が過ぎたところから）。
	// ゲーム内では、生成直後のノーツは画面の外（遠く・上）にあって見えない。
	// その見えない時間を飛ばして、見え始めから判定ラインまでを NoteStartY → NoteEndY に割り当てる
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Note", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float NoteAppearProgress = 0.0f;

	// ============================================================
	// 調整値の計算
	// ============================================================

	// 拍から ±何秒までの入力を集めるか（判定の BadWindow = 0.16 に合わせる）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "0.0", Units = "s"))
	float CaptureRangeSec = 0.160f;

	// 何回ぶん集めたら調整値を更新するか。
	// この回数たまったら平均を調整値に入れ、集めた数を0に戻してまた数え直す（3回ごとに更新）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "1"))
	int32 SamplesPerAdjust = 3;

	// SamplesPerAdjust が5以上のとき、いちばん早い・遅い入力を1つずつ除いて平均する
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust")
	bool bTrimOutliers = true;

	// ============================================================
	// スティック（開いたとき、入力コンポーネントの値で上書きされる）
	// ============================================================

	// これ以上倒したら入力成立
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Stick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FireThreshold = 0.70f;

	// これ以下に戻したら再受付
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Stick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReleaseThreshold = 0.20f;

	// ============================================================
	// 見た目（戻るボタンの色）
	// ============================================================

	// フォーカス中の色
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Color")
	FLinearColor SelectedColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);

	// フォーカスしていないときの色
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Color")
	FLinearColor NormalColor = FLinearColor::White;

	// ============================================================
	// 戻るときのSE
	// ============================================================

	// 曲選択に戻るときに鳴らすSE（WBPのクラスデフォルトに SE_Decide を指定する。未指定なら鳴らさない）。
	// 戻るボタン・A・BackSpace のどれで閉じても鳴る
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|SE")
	TObjectPtr<USoundBase> ReturnSE;

	// ============================================================
	// 入力フィードバック（SE・エフェクト）
	// ============================================================

	// 倒したときに鳴らすSE。初期値は本編と同じ SE_Notes_Perfect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback")
	TObjectPtr<USoundBase> PerfectSE;

	// SEの再生開始位置（SEの頭の無音を飛ばす。本編と同じ 0.12）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback", meta = (ClampMin = "0.0", Units = "s"))
	float PerfectSEStartTime = 0.12f;

	// SEの音量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback", meta = (ClampMin = "0.0"))
	float SEVolume = 1.0f;

	// SE音量スライダーが効くように SCL_SE を入れる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback")
	TObjectPtr<USoundClass> SESoundClass;

	// true: 判定の範囲外（拍から CaptureRangeSec より外れた入力・最初のノーツ前の入力）ではSEを鳴らさない
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback")
	bool bSEOnlyInRange = true;

	// true: 実際にパーフェクト幅に入ったときだけ SE とエフェクトを出す / false: 倒すたびに毎回出す
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback")
	bool bFeedbackOnlyOnPerfect = false;

	// パーフェクト幅（秒）。判定役の PerfectWindow（0.04）に合わせる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback", meta = (ClampMin = "0.0", Units = "s"))
	float PerfectWindowSec = 0.040f;

	// エフェクト画像が消えるまでの秒数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback", meta = (ClampMin = "0.05", Units = "s"))
	float EffectDurationSec = 0.25f;

	// エフェクト画像の大きさ（出た瞬間）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback")
	float EffectStartScale = 0.8f;

	// エフェクト画像の大きさ（消える間際）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimingAdjust|Feedback")
	float EffectEndScale = 1.3f;

	// 倒したときに呼ばれる。Niagara や UMG アニメなど、Blueprint 側で独自の演出を足したいとき用。
	// Lane: 0=左スティック左 / 1=左スティック右 / 2=右スティック左 / 3=右スティック右
	UFUNCTION(BlueprintImplementableEvent, Category = "TimingAdjust|Feedback")
	void OnTimingInput(int32 Lane, bool bPerfect);

	// ============================================================
	// WBP 側に同じ名前の部品を置く（「変数である」にチェック）
	// ============================================================

	// 必須：現在の調整値（数字だけ。例「0ms」「+23ms」）。「現在の調整値：」は WBP に固定テキストで置く
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AdjustValueText;

	// 必須：戻るボタン
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> BackButton;

	// 以下は無くても動く
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> NoteImage;          // 落ちてくるノーツ
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LastErrorText;  // 直前のズレ（例「+12ms」「範囲外」）

	// 倒した方向のエフェクト画像（レーンごと。置いたものだけ光る）
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> EffectLane0;   // 左スティック 左
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> EffectLane1;   // 左スティック 右
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> EffectLane2;   // 右スティック 左
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> EffectLane3;   // 右スティック 右

private:
	UFUNCTION() void HandleBack();

	// 入力を1回受け取り、拍とのズレ計算・演出・ノーツ消去を行う
	void RegisterInput(double InputTime, int32 Lane);

	// 集めたズレの平均（調整値 ms）を求める
	int32 CalcOffsetMs() const;

	// ノーツ画像の位置・表示を更新する
	void UpdateNoteVisual(double Now);

	// 到達時間の進み具合(0〜1)を、画面上の位置の進み具合に変える（遠近感）
	float ApplyDepthEase(float Progress) const;

	// 現在の調整値の表示を更新する
	void RefreshValueText();

	// SE とエフェクトを出す
	void PlayFeedback(int32 Lane, bool bPerfect, bool bInRange);

	// エフェクト画像のフェードアウトを進める
	void UpdateEffects(double Now);

	// レーン番号に対応するエフェクト画像（無ければ nullptr）
	UImage* GetEffectImage(int32 Lane) const;

	class URhythmSettingsSubsystem* GetRhythmSettings() const;

	double StartTime = 0.0;         // 最初の拍の時刻（実時間）
	TArray<float> Samples;          // 今回のぶんとして集めたズレ(ms)。調整値を更新するたびに空に戻る
	bool bLeftArmed = true;         // 左スティックを受け付けられる状態か
	bool bRightArmed = true;        // 右スティックを受け付けられる状態か
	bool bClosed = false;           // 閉じ済みか（二重に閉じない）
	int32 ConsumedBeatIndex = -1;   // 入力で消したノーツの拍番号（-1 = なし）
	double EffectStartTimes[4] = { -1.0, -1.0, -1.0, -1.0 };   // レーンごとのエフェクト開始時刻（-1 = 出ていない）
};