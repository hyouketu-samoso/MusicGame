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
#include "MenuWidgetBase.generated.h"

class UButton;
class USlider;
class USoundBase;
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

	// どのボタンが押されても呼ばれる(決定SE用)
	UFUNCTION()
	void HandleAnyButtonClicked();

	// ChangeSEButtonNames のボタン用(変更SE)
	UFUNCTION()
	void HandleChangeButtonClicked();
};