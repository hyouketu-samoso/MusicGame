#include "RhythmPlayerController.h"
#include "RhythmInputComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BackVideoActor.h"
#include "SongSelectWidget.h"

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
			SongSelectWidget->AddToViewport(100);

			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(SongSelectWidget->TakeWidget());
			SetInputMode(Mode);
			bShowMouseCursor = true;

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

void ARhythmPlayerController::HandlePauseInput()
{
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