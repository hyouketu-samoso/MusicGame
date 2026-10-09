#include "MenuWidgetBase.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "InputCoreTypes.h"

void UMenuWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// Widget内のButtonを全部集める
	MenuButtons.Reset();
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
			{
				if (UButton* Button = Cast<UButton>(Widget))
				{
					MenuButtons.Add(Button);
				}
			});
	}

	// 最初に選択するButtonを決める(名前指定があればそれ、無ければ先頭)
	UButton* FirstButton = nullptr;
	for (UButton* Button : MenuButtons)
	{
		if (Button && InitialButtonName != NAME_None
			&& Button->GetFName() == InitialButtonName)
		{
			FirstButton = Button;
			break;
		}
	}
	if (!FirstButton && MenuButtons.Num() > 0)
	{
		FirstButton = MenuButtons[0];
	}
	if (FirstButton)
	{
		FirstButton->SetKeyboardFocus();
	}
}

void UMenuWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 選択中のButtonだけ色を変える(ポーズ中もUMGのTickは動く)
	for (UButton* Button : MenuButtons)
	{
		if (!Button) continue;

		const bool bFocused = Button->HasKeyboardFocus();

		FLinearColor Color = NormalColor;
		if (bFocused)
		{
			Color = SelectedColor;
		}
		else if (ToggledOnButtons.Contains(TWeakObjectPtr<UButton>(Button)))
		{
			Color = ToggledOnColor;
		}
		Button->SetBackgroundColor(Color);
		

		// フォーカスが新しく移ったら、ブループリントへ通知する
		if (bFocused && LastFocusedButton.Get() != Button)
		{
			LastFocusedButton = Button;
			OnMenuButtonFocused(Button);
		}
	}
}

FReply UMenuWidgetBase::NativeOnPreviewKeyDown(const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	// B(Xbox: B / PS: ○)で、選択中のButtonを実行する
	if (InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right
		&& !InKeyEvent.IsRepeat())
	{
		for (UButton* Button : MenuButtons)
		{
			if (Button && Button->HasKeyboardFocus())
			{
				Button->OnClicked.Broadcast();
				return FReply::Handled();
			}
		}
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UMenuWidgetBase::SetButtonToggledOn(UButton* Button, bool bOn)
{
	if (!Button) return;

	if (bOn)
	{
		ToggledOnButtons.Add(Button);
	}
	else
	{
		ToggledOnButtons.Remove(Button);
	}
}