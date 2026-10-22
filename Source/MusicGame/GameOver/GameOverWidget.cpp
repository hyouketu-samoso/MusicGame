#include "GameOverWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "UObject/UnrealType.h"
#include "RhythmMusicPlayer.h"   // 曲を止めるため

// ボタンを「フォーカスしない」にする
// (エンジンのバージョンでプロパティ名が違っても動くよう、名前で探して設定する)
static void SetGameOverButtonNotFocusable(UButton* Button)
{
	if (!Button)
	{
		return;
	}
	for (const TCHAR* Name : { TEXT("IsFocusable"), TEXT("bIsFocusable") })
	{
		if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(UButton::StaticClass(), Name))
		{
			Prop->SetPropertyValue_InContainer(Button, false);
			return;
		}
	}
}

void UGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// このウィジェット自身がキー入力を受け取る(ボタンはフォーカスさせない)
	SetIsFocusable(true);

	// ボタンは「フォーカスしない」設定にする → UE標準の青い枠線が出ない
	if (TitleButton)
	{
		SetGameOverButtonNotFocusable(TitleButton);
		TitleButton->OnHovered.AddUniqueDynamic(this, &UGameOverWidget::HandleTitleHovered);
		TitleButton->OnClicked.AddUniqueDynamic(this, &UGameOverWidget::HandleTitleClicked);
		ApplyButtonImage(TitleButton, TitleButtonTexture);
		TitleButton->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}
	if (GameEndButton)
	{
		SetGameOverButtonNotFocusable(GameEndButton);
		GameEndButton->OnHovered.AddUniqueDynamic(this, &UGameOverWidget::HandleGameEndHovered);
		GameEndButton->OnClicked.AddUniqueDynamic(this, &UGameOverWidget::HandleGameEndClicked);
		ApplyButtonImage(GameEndButton, GameEndButtonTexture);
		GameEndButton->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}

	// ロゴ画像
	if (LogoImage && LogoTexture)
	{
		LogoImage->SetBrushFromTexture(LogoTexture, true);
	}

	SelectedIndex = 0;
	UpdateVisual();

	// Open() が呼ばれるまでは、見えない状態にしておく
	SetRenderOpacity(0.0f);
}

void UGameOverWidget::NativeDestruct()
{
	RestoreInputMode();
	Super::NativeDestruct();
}

// 画面に出して、操作を受け付け始める
void UGameOverWidget::Open()
{
	if (State != EState::Hidden)
	{
		return;
	}

	if (!IsInViewport())
	{
		AddToViewport(200);   // 曲選択(100)より手前
	}

	// ノーツなどを止める(ポーズ中でも、この画面は動く)
	// ポーズは、レベルを開き直すと自動で解除される
	if (bPauseGameOnOpen)
	{
		UGameplayStatics::SetGamePaused(this, true);
	}

	// 曲を止める
	if (bStopMusicOnOpen)
	{
		if (ARhythmMusicPlayer* Music = Cast<ARhythmMusicPlayer>(
			UGameplayStatics::GetActorOfClass(this, ARhythmMusicPlayer::StaticClass())))
		{
			Music->StopMusic();
		}
	}

	ElapsedFade = 0.0f;
	State = EState::FadingIn;
	SelectedIndex = 0;
	UpdateVisual();
	SetupInputMode();
}

void UGameOverWidget::SetupInputMode()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	bPrevShowCursor = PC->bShowMouseCursor;

	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(TakeWidget());   // ボタンではなく、このウィジェット自身に入力を集める
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = true;           // マウスでも選べる
	bInputModeChanged = true;
}

void UGameOverWidget::RestoreInputMode()
{
	if (!bInputModeChanged)
	{
		return;
	}
	bInputModeChanged = false;

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = bPrevShowCursor;
	}
}

void UGameOverWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (State == EState::FadingIn)
	{
		// ポーズ中でも進むよう、引数の経過時間(ポーズの影響を受けない Tick)を使う
		ElapsedFade += InDeltaTime;
		const float T = FMath::Clamp(ElapsedFade / FMath::Max(FadeInDuration, 0.01f), 0.0f, 1.0f);
		SetRenderOpacity(FMath::InterpEaseOut(0.0f, 1.0f, T, 2.0f));

		if (T >= 1.0f)
		{
			State = EState::Shown;
		}
	}
}

// キー入力(Preview: ボタンより先に受け取る)
FReply UGameOverWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 表示が始まっていないとき、決定後は、何も受け付けない
	if (State != EState::FadingIn && State != EState::Shown)
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}

	const FKey Key = InKeyEvent.GetKey();

	// 左へ: 十字キー左 / 左スティック左 / キーボード ← A
	if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left
		|| Key == EKeys::Left || Key == EKeys::A)
	{
		MoveSelection(-1);
		return FReply::Handled();
	}

	// 右へ: 十字キー右 / 左スティック右 / キーボード → D
	if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right
		|| Key == EKeys::Right || Key == EKeys::D)
	{
		MoveSelection(+1);
		return FReply::Handled();
	}

	// 決定: 設定した決定ボタン(長押しの連続入力は無視) / キーボード Enter
	if ((Key == ConfirmKey || Key == EKeys::Enter) && !InKeyEvent.IsRepeat())
	{
		Confirm();
		return FReply::Handled();
	}

	// 上下・その他は、使わない(UE標準のフォーカス移動に奪われないよう、止める)
	if (Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down
		|| Key == EKeys::Gamepad_LeftStick_Up || Key == EKeys::Gamepad_LeftStick_Down
		|| Key == EKeys::Up || Key == EKeys::Down)
	{
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

// 左右の移動(2つしかないので、端で止める)
void UGameOverWidget::MoveSelection(int32 Direction)
{
	SetSelectedIndex(FMath::Clamp(SelectedIndex + Direction, 0, 1));
}

void UGameOverWidget::SetSelectedIndex(int32 NewIndex)
{
	if (NewIndex == SelectedIndex)
	{
		return;
	}
	SelectedIndex = NewIndex;
	UpdateVisual();
}

// 選択中は大きく・明るく、選んでいないほうは小さく・暗くする
void UGameOverWidget::UpdateVisual()
{
	auto Apply = [this](UButton* Button, bool bSelected)
		{
			if (!Button)
			{
				return;
			}
			Button->SetRenderScale(bSelected ? FVector2D(SelectedScale, SelectedScale) : FVector2D(1.0f, 1.0f));
			Button->SetBackgroundColor(bSelected ? SelectedColor : NormalColor);   // 画像に色を掛ける
		};

	Apply(TitleButton, SelectedIndex == 0);
	Apply(GameEndButton, SelectedIndex == 1);
}

// 決定
void UGameOverWidget::Confirm()
{
	if (State != EState::FadingIn && State != EState::Shown)
	{
		return;
	}
	State = EState::Closing;   // 二重に決定されないようにする

	if (SelectedIndex == 0)
	{
		// TITLE: 通知を送ってから、レベルを開く
		// (入力モードとポーズは、ここでは戻さない。戻すと、レベルが開けなかったときに固まるため。
		//  レベルが開けば、新しいレベルで自動的に元に戻る)
		OnTitleSelected.Broadcast();

		if (TitleLevelName.IsNone())
		{
			// レベル名が未設定: 何も起きないので、画面にも出して、操作できる状態に戻す
			UE_LOG(LogTemp, Error, TEXT("[GameOver] TitleLevelName is not set (WBP_GameOver class defaults)"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
					TEXT("GameOver: Title Level Name が未設定です(WBP_GameOver のクラス既定値)"));
			}
			State = EState::Shown;
			return;
		}

		UGameplayStatics::OpenLevel(this, TitleLevelName);
		State = EState::Shown;   // 開けなかったときも、操作できるようにしておく
	}
	else
	{
		// GAME END: 通知 → ゲームを終了
		OnGameEndSelected.Broadcast();
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
	}
}

// ---- マウス ----
void UGameOverWidget::HandleTitleHovered() { SetSelectedIndex(0); }
void UGameOverWidget::HandleGameEndHovered() { SetSelectedIndex(1); }

void UGameOverWidget::HandleTitleClicked()
{
	SelectedIndex = 0;
	Confirm();
}

void UGameOverWidget::HandleGameEndClicked()
{
	SelectedIndex = 1;
	Confirm();
}

// ボタンに画像を設定する(通常・乗せた・押した、すべて同じ画像。枠線・背景色なし)
void UGameOverWidget::ApplyButtonImage(UButton* Button, UTexture2D* Texture)
{
	if (!Button || !Texture)
	{
		return;
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());

	FButtonStyle Style = Button->GetStyle();
	Style.Normal = Brush;
	Style.Hovered = Brush;
	Style.Pressed = Brush;
	Style.Disabled = Brush;
	Style.NormalPadding = FMargin(0.0f);
	Style.PressedPadding = FMargin(0.0f);
	Button->SetStyle(Style);

}