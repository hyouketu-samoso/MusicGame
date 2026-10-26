// ============================================================
// GameOverWidget
//  役割 : ゲームオーバー画面(ロゴ画像 + TITLE / GAME END の2ボタン)
//  操作 : 左右(十字キー・左スティック・キーボードの←→/A/D)で選択を移動
//         決定ボタン(初期値は ○ = Gamepad_FaceButton_Right)で決定
//         マウスでも、乗せて選択・クリックで決定できる
//  表示 : 文字は使わず、すべて画像。選択中のボタンは大きく・明るく見せる
//  ※ ボタンは「フォーカスしない」設定にして、選択はコードで管理している
//     (UE標準の青い枠線が出ない)
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "GameOverWidget.generated.h"

class UButton;
class UImage;
class UTexture2D;

// ボタンが決定されたときの通知(ほかのプログラマ用)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOverTitleSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOverEndSelected);

UCLASS()
class MUSICGAME_API UGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ゲームオーバー画面を表示して、操作を受け付け始める
	// (ゲームオーバーになった場所から呼ぶ。WBP を CreateWidget した後に呼ぶ)
	UFUNCTION(BlueprintCallable, Category = "GameOver")
	void Open();

	// TITLE が決定されたとき
	UPROPERTY(BlueprintAssignable, Category = "GameOver")
	FOnGameOverTitleSelected OnTitleSelected;

	// GAME END が決定されたとき
	UPROPERTY(BlueprintAssignable, Category = "GameOver")
	FOnGameOverEndSelected OnGameEndSelected;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// ---- 設定(WBP のクラス既定値で変えられる) ----

	// TITLE を決定したときに開くレベル名(空なら、通知だけ送る)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Setting")
	FName TitleLevelName;

	// 決定ボタン(○ = Right / A(Xbox) = Bottom に変えられる)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Setting")
	FKey ConfirmKey = EKeys::Gamepad_FaceButton_Right;

	// 表示するとき、ゲームを一時停止する(ノーツ・判定などが止まる)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Setting")
	bool bPauseGameOnOpen = true;

	// 表示するとき、曲を止める
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Setting")
	bool bStopMusicOnOpen = true;

	// 見えるまでの秒数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Setting", meta = (ClampMin = "0.0"))
	float FadeInDuration = 0.5f;

	// 選択中のボタンの大きさ・色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Setting")
	float SelectedScale = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Setting")
	FLinearColor SelectedColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Setting")
	FLinearColor NormalColor = FLinearColor(0.55f, 0.55f, 0.55f, 1.0f);

	// ---- 画像(デザイナーの素材ができたら差し替える) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Image")
	TObjectPtr<UTexture2D> LogoTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Image")
	TObjectPtr<UTexture2D> TitleButtonTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver|Image")
	TObjectPtr<UTexture2D> GameEndButtonTexture;

	// ---- WBP の部品(名前を合わせる) ----
	// ①ロゴ画像(Image)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> LogoImage;

	// ②TITLE ボタン(Button)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TitleButton;

	// ③GAME END ボタン(Button)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GameEndButton;

	// ボタンの見た目用の画像(Image。無くても動く)
	//  置いたときは、ボタンは「押せる範囲」だけになり(見えなくなり)、絵はこの画像が受け持つ。
	//  画像の大きさを、ボタンの範囲と別に決められる(ボタンの上に重ねて置く)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TitleButtonImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> GameEndButtonImage;

private:
	enum class EState : uint8 { Hidden, FadingIn, Shown, Closing };

	// 選択の移動(Direction: -1 = 左 / +1 = 右)
	void MoveSelection(int32 Direction);

	// 選択を決める(マウスの乗せ・移動の共通処理)
	void SetSelectedIndex(int32 NewIndex);

	// 選択中のボタンを見た目に反映する
	void UpdateVisual();

	// 決定
	void Confirm();

	UFUNCTION() void HandleTitleHovered();
	UFUNCTION() void HandleGameEndHovered();
	UFUNCTION() void HandleTitleClicked();
	UFUNCTION() void HandleGameEndClicked();

	// ボタンを、見えなくする(押せる範囲だけ残す。見た目用の画像を別に置いたとき用)
	void MakeButtonInvisible(UButton* Button);

	// ボタンに画像を設定する(枠なし・文字なし)
	void ApplyButtonImage(UButton* Button, UTexture2D* Texture);

	// 入力を UI 用にする / ゲーム操作に戻す
	void SetupInputMode();
	void RestoreInputMode();

	int32 SelectedIndex = 0;   // 0 = TITLE / 1 = GAME END
	EState State = EState::Hidden;
	float ElapsedFade = 0.0f;
	bool bInputModeChanged = false;
	bool bPrevShowCursor = false;
};