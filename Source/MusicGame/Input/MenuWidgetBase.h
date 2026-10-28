// ============================================================
// MenuWidgetBase
//  役割 : メニュー画面の共通基底。Pad/キーボードで項目を選んで実行する
//  移動 : 十字キー / 左スティック / 矢印キー(UMG標準のフォーカス移動)
//  決定 : B(Gamepad_FaceButton_Right)で選択中のButtonを実行
//         ※ A / Enter / クリックも標準で決定になる
//  表示 : 選択中のButtonを SelectedColor にする
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "Styling/SlateTypes.h"
#include "MenuWidgetBase.generated.h"

// スライダーを動かせる状態(編集中)のとき、横の画像を差し替えるための設定1件分
USTRUCT(BlueprintType)
struct FMenuSliderEditImage
{
	GENERATED_BODY()

	// 対象のスライダーの名前(WBPのSliderと同じ名前)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName SliderName;

	// 差し替える Image の名前(WBPのImageと同じ名前)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName ImageName;

	// スライダーを動かせる状態(編集中)の間に表示する画像
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TObjectPtr<UTexture2D> EditingTexture;
};

class UButton;
class USlider;
class USoundBase;
class UImage;
class UTexture2D;
class FMenuAdjustKeyTracker;

UCLASS()
class MUSICGAME_API UMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;
	virtual void NativeDestruct() override;

	// 選択中のButtonの色
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	FLinearColor SelectedColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);

	// フォーカス中のButtonの色を SelectedColor に変えるか(全ボタン共通の基本設定)
	// ポーズ画面など、色を変えたくないWBPでは OFF にする
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	bool bChangeButtonColorOnFocus = true;

	// 上のフラグが OFF でも、フォーカス時に色を変えたいボタンの名前(WBPのButtonと同じ名前)
	// 例: ポーズ画面のタブボタン4つだけ色を変えたいとき
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	TArray<FName> FocusColorButtonNames;

	// 「最後にフォーカスが当たったボタン」を選択中として残すボタンの名前(タブボタンなど)。
	// このリストの中で別のボタンにフォーカスが移るまで、フォーカスが外れても選択中の見た目
	// (Hoveredの画像・選択色)を保つ。パネルを開いてフォーカスがタブの外へ移っても、そのタブは緑のまま
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	TArray<FName> StickyHighlightButtonNames;

	// これらのボタンにフォーカスが移ったら、残していた「選択中」を解除する(タブを元の色に戻す)。
	// ゲーム再開・ゲーム終了など、パネルの外にあるボタンの名前を入れる
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	TArray<FName> StickyReleaseButtonNames;

	// パッド/キーボードでフォーカスが合ったButtonも、マウスを乗せたとき(Hovered)と同じ画像にするか。
	// ButtonのStyleの Hovered に入れた画像が、フォーカス中の Normal として表示される
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	bool bUseHoveredStyleOnFocus = true;

	// スライダーを動かせる状態(編集中)のとき、横の画像を別の画像に差し替える設定(スライダーごとに1件)
	// ※ SliderのRequires Controller Lockを有効にしておく。右ボタンで編集開始、もう一度押すと終了
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	TArray<FMenuSliderEditImage> SliderEditImages;

	// 選択されていないButtonの色
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	FLinearColor NormalColor = FLinearColor::White;

	// 最初に選択するButtonの名前(WBPのButtonと同じ名前にする)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	FName InitialButtonName = NAME_None;

	// フォーカスが別のボタンに移った瞬間に呼ばれる(パッドの十字キー/スティック用)
	UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
	void OnMenuButtonFocused(UButton* FocusedButton);

	// 下(A)が押されたときに呼ばれる（戻る）。何をするかは WBP 側で決める
	UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
	void OnMenuBackRequested();

	// ON状態のButtonの色(フォーカスされていない時)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	FLinearColor ToggledOnColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);


	// ---- SE(WBPごとに指定する。未指定なら鳴らさない) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	TObjectPtr<USoundBase> MoveSE;      // カーソル移動

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	TObjectPtr<USoundBase> DecideSE;   // 決定

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	TObjectPtr<USoundBase> BackSE;      // 戻る

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	TObjectPtr<USoundBase> ChangeSE;      // 変更

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	TObjectPtr<USoundBase> OpenSE;      // 開く
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	TObjectPtr<USoundBase> CloseSE;      // 閉じる

	// 決定SE(DecideSE)を鳴らさないボタンの名前(WBPのButtonと同じ名前)。
	// タブボタンなど、決定時に別のSE(OpenSEなど)を鳴らしたいボタンを入れる
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	TArray<FName> NoDecideSEButtonNames;

	// 決定したとき、DecideSE の代わりに ChangeSE を鳴らすボタンの名前
	// (左右ボタン、ON/OFFボタンなど「値を変える」ボタン用)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	TArray<FName> ChangeSEButtonNames;

	// スライダーの ChangeSE は、左スティックを戻した(ニュートラルに戻った)ときに1回だけ鳴らす。
	// これは、最後に値が変わってから鳴らすまでの最短の待ち時間(秒)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	float ChangeSEDelay = 0.1f;

	// スティックを「倒している」とみなすしきい値(WBPの加速処理と同じ 0.5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|SE")
	float StickNeutralThreshold = 0.5f;

public:
	// 指定したButtonを「ON状態の色」にするか切り替える(WBPから呼ぶ)
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SetButtonToggledOn(UButton* Button, bool bOn);

	// SEを鳴らす(ポーズ中でも鳴る)。WBPから任意のSEを鳴らしたいときにも使える
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void PlayMenuSE(USoundBase* SE);

private:
	UPROPERTY()
	TArray<TObjectPtr<UButton>> MenuButtons;

	// スライダーもフォーカス移動でMoveSEを鳴らすため集めておく
	UPROPERTY()
	TArray<TObjectPtr<USlider>> MenuSliders;

	TWeakObjectPtr<UButton> LastFocusedButton;
	TWeakObjectPtr<USlider> LastFocusedSlider;
	// 一度でもフォーカスが付いたか(開いた直後の最初のフォーカスでは鳴らさないため)
	bool bHasHadFocus = false;

	// スライダーの前フレームの値(整数)。変化を見つけてChangeSEを鳴らすため
	TMap<TWeakObjectPtr<USlider>, int32> SliderLastValues;
	// スライダーの値が最後に変わった時刻。bChangeSEPending が true の間、変化が止まるのを待つ
	double LastSliderChangeTime = 0.0;
	bool bChangeSEPending = false;

	// 値を変える入力(十字キーの左右・矢印キーの左右)の押下状態を、
	// Slateの入力プリプロセッサで直接見張る(スライダーがキーを先に処理しても取りこぼさない)。
	// 押している間は ChangeSE を鳴らさず、離してから鳴らす
	TSharedPtr<FMenuAdjustKeyTracker> AdjustKeyTracker;
	TSet<TWeakObjectPtr<UButton>> ToggledOnButtons;

	// フォーカス時のStyle差し替え用。元のStyleを覚えておいて、フォーカスが外れたら戻す
	TMap<TWeakObjectPtr<UButton>, FButtonStyle> OriginalButtonStyles;
	TSet<TWeakObjectPtr<UButton>> StyleSwappedButtons;

	// StickyHighlightButtonNames の中で、いま「選択中」として残しているボタン
	TWeakObjectPtr<UButton> StickyHighlightedButton;

	// スライダー横の画像の差し替え用(実行中の管理データ)
	struct FSliderImageRuntime
	{
		TWeakObjectPtr<USlider> Slider;
		TWeakObjectPtr<UImage> Image;
		FSlateBrush OriginalBrush;   // 差し替える前の画像
		int32 ConfigIndex = INDEX_NONE;   // SliderEditImages の何番目か
		bool bEditing = false;       // スライダーが編集中(動かせる状態)か
		bool bApplied = false;       // 今、差し替え後の画像を表示しているか
	};
	TArray<FSliderImageRuntime> SliderImageRuntimes;

	// どのボタンが押されても呼ばれる(決定SE用)
	UFUNCTION()
	void HandleAnyButtonClicked();

	// ChangeSEButtonNames のボタン用(変更SE)
	UFUNCTION()
	void HandleChangeButtonClicked();

	// スライダーの編集開始/終了(Requires Controller Lock)で呼ばれる
	UFUNCTION()
	void HandleSliderControllerCaptureBegin();

	UFUNCTION()
	void HandleSliderControllerCaptureEnd();
};