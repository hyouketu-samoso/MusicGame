#include "MenuWidgetBase.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Blueprint/WidgetTree.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/NavigationConfig.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"
#include "Framework/Application/IInputProcessor.h"
#include "GenericPlatform/ICursor.h"

// 十字キー/矢印キーの左右が押されているかを、ウィジェットより手前で見張る。
// (スライダーがキーを処理してしまっても、押下/解放のイベントを取りこぼさない)
class FMenuAdjustKeyTracker : public IInputProcessor
{
public:
	TSet<FKey> Held;
	double LastEventTime = 0.0;

	static bool IsAdjustKey(const FKey& Key)
	{
		return Key == EKeys::Gamepad_DPad_Left
			|| Key == EKeys::Gamepad_DPad_Right
			|| Key == EKeys::Left
			|| Key == EKeys::Right;
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		if (IsAdjustKey(InKeyEvent.GetKey()))
		{
			Held.Add(InKeyEvent.GetKey());
			LastEventTime = FPlatformTime::Seconds();
		}
		return false; // 入力は消費しない
	}

	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		if (IsAdjustKey(InKeyEvent.GetKey()))
		{
			Held.Remove(InKeyEvent.GetKey());
		}
		return false; // 入力は消費しない
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("MenuAdjustKeyTracker"); }
};

void UMenuWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// Widget内のButtonを全部集める
	MenuButtons.Reset();
	MenuSliders.Reset();
	SliderLastValues.Reset();

	// 十字キー/矢印キーの押下状態の見張りを登録する
	if (FSlateApplication::IsInitialized())
	{
		if (AdjustKeyTracker.IsValid())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(AdjustKeyTracker);
		}
		AdjustKeyTracker = MakeShared<FMenuAdjustKeyTracker>();
		FSlateApplication::Get().RegisterInputPreProcessor(AdjustKeyTracker);
	}
	LastFocusedButton.Reset();   // 開き直したとき、最初のフォーカスで移動SEが鳴らないようにする
	LastFocusedSlider.Reset();
	bHasHadFocus = false;
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
			{
				if (UButton* Button = Cast<UButton>(Widget))
				{
					MenuButtons.Add(Button);
					// 決定SE用(右ボタン・Enter・クリックのどれでも鳴る)
					// ただし NoDecideSEButtonNames に入っているボタンは鳴らさない
					// ChangeSEButtonNames に入っているボタンは、決定SEの代わりに変更SEを鳴らす
					if (ChangeSEButtonNames.Contains(Button->GetFName()))
					{
						Button->OnClicked.AddUniqueDynamic(this, &UMenuWidgetBase::HandleChangeButtonClicked);
					}
					else if (!NoDecideSEButtonNames.Contains(Button->GetFName()))
					{
						Button->OnClicked.AddUniqueDynamic(this, &UMenuWidgetBase::HandleAnyButtonClicked);
					}
				}
				else if (USlider* Slider = Cast<USlider>(Widget))
				{
					MenuSliders.Add(Slider);
					SliderLastValues.Add(Slider, FMath::RoundToInt(Slider->GetValue()));
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

	// 右ボタン(B)を「決定」に追加する。
	// スライダーの編集開始/終了(Requires Controller Lock)がこのキーで動く
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetNavigationConfig()->KeyActionRules.Add(
			EKeys::Gamepad_FaceButton_Right, EUINavigationAction::Accept);
	}
}

void UMenuWidgetBase::NativeDestruct()
{
	if (AdjustKeyTracker.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(AdjustKeyTracker);
	}
	AdjustKeyTracker.Reset();

	Super::NativeDestruct();
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
			// 最初のフォーカス(メニューを開いた直後)では鳴らさない
			const bool bHadPrevious = bHasHadFocus;
			bHasHadFocus = true;
			LastFocusedButton = Button;
			LastFocusedSlider.Reset();
			if (bHadPrevious)
			{
				PlayMenuSE(MoveSE);
			}
			OnMenuButtonFocused(Button);
		}
	}

	// スライダーにフォーカスが移ったときも移動SEを鳴らす
	for (USlider* Slider : MenuSliders)
	{
		if (!Slider) continue;

		if (Slider->HasKeyboardFocus() && LastFocusedSlider.Get() != Slider)
		{
			const bool bHadPrevious = bHasHadFocus;
			bHasHadFocus = true;
			LastFocusedSlider = Slider;
			LastFocusedButton.Reset();
			if (bHadPrevious)
			{
				PlayMenuSE(MoveSE);
			}
		}
	}

	// スライダーの値(整数)が変わったら「動かしている途中」とみなし、
	// 変化が ChangeSEDelay 秒止まったら変更SEを1回だけ鳴らす。
	// フォーカスが無いときの変化(画面を開いたときの初期値セットなど)は対象外
	const double Now = FPlatformTime::Seconds();
	for (USlider* Slider : MenuSliders)
	{
		if (!Slider) continue;

		const int32 NowValue = FMath::RoundToInt(Slider->GetValue());
		int32& LastValue = SliderLastValues.FindOrAdd(Slider, NowValue);
		if (NowValue != LastValue)
		{
			LastValue = NowValue;
			if (Slider->HasKeyboardFocus())
			{
				bChangeSEPending = true;
				LastSliderChangeTime = Now;
			}
		}
	}

	// 加速中の「間」(最初の1段のあと少し止まる時間)では鳴らさないよう、
	// スティックを戻してから鳴らす
	// 保険: ウィンドウのフォーカスが外れたなどで離したイベントが来なかった場合に備える。
	// 押している間は繰り返しイベントが来るので、1.5秒来なければ離したとみなす
	if (AdjustKeyTracker.IsValid() && AdjustKeyTracker->Held.Num() > 0
		&& Now - AdjustKeyTracker->LastEventTime > 1.5)
	{
		AdjustKeyTracker->Held.Reset();
	}

	if (bChangeSEPending && Now - LastSliderChangeTime >= ChangeSEDelay)
	{
		bool bStickNeutral = true;
		const bool bAdjustKeyHeld = AdjustKeyTracker.IsValid() && AdjustKeyTracker->Held.Num() > 0;
		if (const APlayerController* PC = GetOwningPlayer())
		{
			bStickNeutral = FMath::Abs(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX)) <= StickNeutralThreshold;
		}

		if (bStickNeutral && !bAdjustKeyHeld)
		{
			bChangeSEPending = false;
			PlayMenuSE(ChangeSE);
		}
	}
}

FReply UMenuWidgetBase::NativeOnPreviewKeyDown(const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	// 下(A)は「戻る」。ボタンは押さず、WBP にイベントで知らせる
	if (InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (!InKeyEvent.IsRepeat())
		{
			PlayMenuSE(BackSE);
			OnMenuBackRequested();
		}
		return FReply::Handled();
	}

	// Bの押しっぱなし（リピート）は受け取らない。
	// 別の画面から戻った直後に B を押したままだと、リピートの入力がフォーカス中のButtonに届き、
	// Button が「押された状態」になって、B を離したときに勝手に決定されてしまうため
	if (InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right
		&& InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}

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

		// スライダーにフォーカスがあるときは、編集の開始/終了なので決定SEだけ鳴らす。
		// 編集そのものの処理は WBP 側(Super経由)に任せる
		for (USlider* Slider : MenuSliders)
		{
			if (Slider && Slider->HasKeyboardFocus())
			{
				PlayMenuSE(DecideSE);
				break;
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

//
void UMenuWidgetBase::HandleAnyButtonClicked()
{
	PlayMenuSE(DecideSE);
}

void UMenuWidgetBase::HandleChangeButtonClicked()
{
	PlayMenuSE(ChangeSE);
}

void UMenuWidgetBase::PlayMenuSE(USoundBase* SE)
{
	if (SE)
	{
		// bIsUISound=true(初期値)なので、ポーズ中でも鳴る
		UGameplayStatics::PlaySound2D(this, SE);
	}
}