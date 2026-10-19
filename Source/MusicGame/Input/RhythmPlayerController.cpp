#include "RhythmPlayerController.h"
#include "RhythmInputComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BackVideoActor.h"
#include "SongSelectWidget.h"
#include "TimingAdjustWidget.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ARhythmPlayerController::ARhythmPlayerController()
{
	RhythmInput =
		CreateDefaultSubobject<URhythmInputComponent>(TEXT("RhythmInput"));
}

void ARhythmPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// ポーズ入力を受け取る
	if (RhythmInput)
	{
		RhythmInput->OnPauseInput.AddDynamic(this, &ARhythmPlayerController::HandlePauseInput);
	}

	// ゲーム画面のWidgetを表示する
	if (IsLocalPlayerController() && MainWidgetClass)
	{
		MainWidget = CreateWidget<UUserWidget>(this, MainWidgetClass);
		if (MainWidget)
		{
			MainWidget->AddToViewport();
		}
	}

	if (IsLocalPlayerController() && BackVideoClass)
	{
		BackVideo = GetWorld()->SpawnActor<ABackVideoActor>(BackVideoClass, BackVideoTransform);
	}

	// 曲選択を、一番手前に出す
	if (IsLocalPlayerController() && SongSelectWidgetClass)
	{
		SongSelectWidget = CreateWidget<USongSelectWidget>(this, SongSelectWidgetClass);
		if (SongSelectWidget)
		{
			SongSelectWidget->OnClosed.AddDynamic(this, &ARhythmPlayerController::HandleSongSelectClosed);
			SongSelectWidget->OnTimingAdjustRequested.AddDynamic(this, &ARhythmPlayerController::HandleTimingAdjustRequested);
			SongSelectWidget->AddToViewport(100);

			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(SongSelectWidget->GetDefaultFocusWidget());
			SetInputMode(Mode);
			SongSelectWidget->FocusDefaultButton();
			bShowMouseCursor = false;

			// 曲を選択しているときは時間を止める
			SetPause(true);
		}
	}
}

void ARhythmPlayerController::HandleSongSelectClosed()
{
	// 曲選択時に止めていた時間を動くようにする
	SetPause(false);

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	SongSelectWidget = nullptr;
}

void ARhythmPlayerController::HandleTimingAdjustRequested()
{
	if (!SongSelectWidget || !TimingAdjustWidgetClass || TimingAdjustWidget)
	{
		return;
	}

	TimingAdjustWidget = CreateWidget<UTimingAdjustWidget>(this, TimingAdjustWidgetClass);
	if (!TimingAdjustWidget)
	{
		return;
	}

	TimingAdjustWidget->OnClosed.AddDynamic(this, &ARhythmPlayerController::HandleTimingAdjustClosed);

	// 曲選択は隠しておく（下のボタンを押せないようにする）
	SongSelectWidget->SetVisibility(ESlateVisibility::Collapsed);
	TimingAdjustWidget->AddToViewport(200);

	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(TimingAdjustWidget->TakeWidget());
	SetInputMode(Mode);
}

void ARhythmPlayerController::HandleTimingAdjustClosed()
{
	TimingAdjustWidget = nullptr;

	if (!SongSelectWidget)
	{
		return;
	}

	SongSelectWidget->SetVisibility(ESlateVisibility::Visible);

	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(SongSelectWidget->GetDefaultFocusWidget());
	SetInputMode(Mode);
	SongSelectWidget->FocusDefaultButton();
}

void ARhythmPlayerController::HandlePauseInput()
{
	if (bPauseInputBlocked || FPlatformTime::Seconds() < PauseInputIgnoreUntil)
	{
		return;
	}
	// 曲選択が出ている間は、ポーズを受け付けない
	if (SongSelectWidget)
	{
		return;
	}
	TogglePause();
}

//void ARhythmPlayerController::HandlePauseInput()
//{
//	TogglePause();
//}

void ARhythmPlayerController::TogglePause()
{
	if (bIsPaused) {
		ResumeGame();
	}
	else {
		OpenPause();
	}
}

void ARhythmPlayerController::OpenPause()
{
	if (bIsPaused) return;

	// Widgetがまだ無ければ作る
	if (!PauseWidget && PauseWidgetClass)
	{
		PauseWidget = CreateWidget<UUserWidget>(this, PauseWidgetClass);
	}

	if (PauseWidget)
	{
		PauseWidget->AddToViewport(100);

		// ポーズを開いたSE(UIサウンドなのでポーズ中も鳴る)
		if (PauseOpenSE)
		{
			UGameplayStatics::PlaySound2D(this, PauseOpenSE);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PauseWidgetClass is not set. Set it in BP_RhythmPlayerController."));
	}

	// マウスカーソルを出して、UIも操作できるようにする
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	SetPause(true);
	bIsPaused = true;
}

void ARhythmPlayerController::ResumeGame()
{
	if (!bIsPaused)return;

	if (PauseWidget) {
		PauseWidget->RemoveFromParent();
	}

	// ゲーム入力に戻す
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	SetPause(false);
	bIsPaused = false;
}

void ARhythmPlayerController::QuitToWindows()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ARhythmPlayerController::SetPauseInputBlocked(bool bBlocked)
{
	bPauseInputBlocked = bBlocked;

	// 解除した直後は、同じ Esc の入力が残っているので、少しの間だけ無視する
	if (!bBlocked)
	{
		PauseInputIgnoreUntil = FPlatformTime::Seconds() + 0.3;
	}
}