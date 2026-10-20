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
#include "RhythmMusicPlayer.h"
#include "SongData.h"
#include "Rhythm/NoteSpawner.h"
#include "Rhythm/RhythmSettingsSubsystem.h"
#include "Engine/GameInstance.h"

ARhythmPlayerController::ARhythmPlayerController()
{
	RhythmInput =
		CreateDefaultSubobject<URhythmInputComponent>(TEXT("RhythmInput"));
}

void ARhythmPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// レベル内のRhythmMusicPlayerを取得
	MusicPlayer = Cast<ARhythmMusicPlayer>(
		UGameplayStatics::GetActorOfClass(
			this,
			ARhythmMusicPlayer::StaticClass()
		)
	);

	// レベル内のNoteSpawnerを取得
	NoteSpawner = Cast<ANoteSpawner>(
		UGameplayStatics::GetActorOfClass(
			this,
			ANoteSpawner::StaticClass()
		)
	);

	if (!NoteSpawner)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[PlayerController] NoteSpawner not found")
		);
	}

	if (!MusicPlayer)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[PlayerController] MusicPlayer not found")
		);
	}

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

			// 曲変更ボタンの通知を受け取る
			SongSelectWidget->OnSongChangeRequested.AddDynamic(
				this,
				&ARhythmPlayerController::HandleSongChangeRequested
			);

			// スタートボタンの通知を受け取る
			SongSelectWidget->OnStartRequested.AddDynamic(
				this,
				&ARhythmPlayerController::HandleStartRequested
			);
			// 最初に選択されている曲名を表示
			RefreshSongSelectInfo();

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
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[PlayerController] SongSelectClosed called")
	);

	// 曲選択時に止めていた時間を動くようにする
	SetPause(false);

	/*if (bStartRequested)
	{
		bStartRequested = false;

		if (MusicPlayer)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[PlayerController] Calling PlayMusic")
			);

			MusicPlayer->PlayMusic();
		}
		else
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[PlayerController] MusicPlayer is null")
			);
		}
	}*/
	if (bStartRequested)
	{
		bStartRequested = false;

		// 必要なオブジェクトを確認
		if (!MusicPlayer || !NoteSpawner)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[PlayerController] MusicPlayer or NoteSpawner is null")
			);

			SetInputMode(FInputModeGameOnly());
			bShowMouseCursor = false;
			SongSelectWidget = nullptr;
			return;
		}

		// 選択中の曲データを取得
		USongData* SongData = MusicPlayer->GetCurrentSongData();

		if (!SongData)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[PlayerController] Current SongData is null")
			);

			SetInputMode(FInputModeGameOnly());
			bShowMouseCursor = false;
			SongSelectWidget = nullptr;
			return;
		}

		// 選択中の難易度を取得
		FString SelectedChartPath;

		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (const URhythmSettingsSubsystem* Settings =
				GameInstance->GetSubsystem<URhythmSettingsSubsystem>())
			{
				if (Settings->GetDifficulty() == ERhythmDifficulty::Hard)
				{
					SelectedChartPath = SongData->HardChartFilePath;
				}
				else
				{
					SelectedChartPath = SongData->NormalChartFilePath;
				}
			}
		}

		// 譜面のパスが設定されているか確認
		if (SelectedChartPath.IsEmpty())
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[PlayerController] Selected chart path is empty")
			);

			SetInputMode(FInputModeGameOnly());
			bShowMouseCursor = false;
			SongSelectWidget = nullptr;
			return;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[PlayerController] Selected chart: %s"),
			*SelectedChartPath
		);

		// 選択した譜面を読み込む
		if (!NoteSpawner->LoadChartFromPath(SelectedChartPath))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[PlayerController] Chart loading failed")
			);

			SetInputMode(FInputModeGameOnly());
			bShowMouseCursor = false;
			SongSelectWidget = nullptr;
			return;
		}

		// 譜面の読み込み成功後に開始する
		NoteSpawner->StartChart();
		MusicPlayer->PlayMusic();

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[PlayerController] Chart and music started")
		);
	}

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

void ARhythmPlayerController::HandleStartRequested()
{
	bStartRequested = true;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[PlayerController] Start requested")
	);
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

void ARhythmPlayerController::RefreshSongSelectInfo()
{
	// WidgetとMusicPlayerが存在するか確認
	if (!SongSelectWidget || !MusicPlayer)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[PlayerController] SongSelectWidget or MusicPlayer is null")
		);
		return;
	}

	// 現在選択されているSongDataを取得
	USongData* SongData = MusicPlayer->GetCurrentSongData();

	if (!SongData)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[PlayerController] Current song data is null")
		);
		return;
	}

	// SongDataの曲名をWidgetに表示
	SongSelectWidget->SetSongTitle(SongData->SongTitle);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[PlayerController] Song title updated: %s"),
		*SongData->SongTitle.ToString()
	);
}

void ARhythmPlayerController::HandleSongChangeRequested(int32 Direction)
{
	if (!MusicPlayer)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[PlayerController] MusicPlayer is null")
		);
		return;
	}

	// 曲番号を変更
	MusicPlayer->HandleSongChangeRequested(Direction);

	// 変更後の曲名を表示
	RefreshSongSelectInfo();
}