#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimingAdjustWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimingAdjustClosed);

UCLASS()
class MUSICGAME_API UTimingAdjustWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "TimingAdjust")
	FOnTimingAdjustClosed OnClosed;

	UFUNCTION(BlueprintCallable, Category = "TimingAdjust")
	void Close();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;

	// ノーツが判定ラインに届く間隔（秒）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "0.5", Units = "s"))
	float BeatIntervalSec = 1.0f;

	// ノーツが出現してから判定ラインに届くまでの秒数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "0.2", Units = "s"))
	float NoteTravelSec = 1.0f;

	// 判定ラインを通り過ぎたあと、見えたままにする秒数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "0.0", Units = "s"))
	float PassVisibleSec = 0.2f;

	// 拍から ±何秒までの入力を集めるか（判定の BadWindow = 0.16 に合わせる）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "0.0", Units = "s"))
	float CaptureRangeSec = 0.160f;

	// 平均に使う直近の入力数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "1"))
	int32 MaxSamples = 8;

	// この回数たまるまでは調整値を変えない
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust", meta = (ClampMin = "1"))
	int32 MinSamplesToApply = 3;

	// 5回以上たまったら、いちばん早い・遅い入力を1つずつ除いて平均する
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust")
	bool bTrimOutliers = true;

	// テスト用：D/F/J/K でも調整できる
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust")
	bool bAcceptKeyboard = true;

	// 戻るボタンの色（フォーカス中 / それ以外）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Color")
	FLinearColor SelectedColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Color")
	FLinearColor NormalColor = FLinearColor::White;

	// NoteImage の Render Translation の Y（出現位置 → 判定ライン、ピクセル）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Note")
	float NoteStartY = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Note")
	float NoteEndY = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Stick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FireThreshold = 0.70f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimingAdjust|Stick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReleaseThreshold = 0.20f;

	// ---- WBP側に同じ名前の部品を置く（「変数である」にチェック） ----
	// 数字だけ（例「0ms」「+23ms」）。「現在の調整値：」は WBP に固定テキストで置く
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> AdjustValueText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> BackButton;

	// 無くても動く
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> NoteImage;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LastErrorText;

private:
	UFUNCTION() void HandleBack();

	void RegisterInput(double InputTime);
	int32 CalcOffsetMs() const;
	void UpdateNoteVisual(double Now);
	void RefreshValueText();

	class URhythmSettingsSubsystem* GetRhythmSettings() const;

	double StartTime = 0.0;       // 最初の拍の時刻（実時間）
	TArray<float> Samples;        // 集めたズレ(ms)。古い順
	bool bLeftArmed = true;
	bool bRightArmed = true;
	bool bClosed = false;
};