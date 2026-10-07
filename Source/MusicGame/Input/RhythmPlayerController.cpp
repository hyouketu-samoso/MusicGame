#include "RhythmPlayerController.h"
#include "RhythmInputComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

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
}

void ARhythmPlayerController::HandlePauseInput()
{
	TogglePause();
}

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