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
#include "MenuWidgetBase.generated.h"

class UButton;

UCLASS()
class MUSICGAME_API UMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

	// 選択中のButtonの色
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	FLinearColor SelectedColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);

	// 選択されていないButtonの色
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	FLinearColor NormalColor = FLinearColor::White;

	// 最初に選択するButtonの名前(WBPのButtonと同じ名前にする)
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category = "Menu")
	FName InitialButtonName = NAME_None;

	// フォーカスが別のボタンに移った瞬間に呼ばれる(パッドの十字キー/スティック用)
	UFUNCTION(BlueprintImplementableEvent,Category = "Menu")
	void OnMenuButtonFocused(UButton* FocusedButton);

private:
	UPROPERTY()
	TArray<TObjectPtr<UButton>> MenuButtons;
	TWeakObjectPtr<UButton> LastFocusedButton;
};