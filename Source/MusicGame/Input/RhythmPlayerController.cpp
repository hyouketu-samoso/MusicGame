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
#include "GameSessionSubsystem.h"   // リトライで曲選択を飛ばす印と、前回の曲番号
#include "TimerManager.h"

ARhythmPlayerController::ARhythmPlayerController()
{
	RhythmInput =
		CreateDefaultSubobject<URhythmInputComponent>(TEXT("RhythmInput"));
}

bool ARhythmPlayerController::IsRetryStart()
{
	if (!bRetryChecked)
	{
		bRetryChecked = true;
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UGameSessionSubsystem* Session = GI->GetSubsystem<UGameSessionSubsystem>())
			{
				bRetryStartValue = Session->ConsumeSkipSongSelect();
			}
		}
	}
	return bRetryStartValue;
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

	// 背景動画
	if (IsLocalPlayerController())
	{
		if (BackVideoClass)
		{
			BackVideo = GetWorld()->SpawnActor<ABackVideoActor>(BackVideoClass, BackVideoTransform);
		}
		else
		{
			// スポーンしない設定のときは、レベルに置いてあるものを使う
			BackVideo = Cast<ABackVideoActor>(
				UGameplayStatics::GetActorOfClass(this, ABackVideoActor::StaticClass()));
		}
	}

	// リトライで開き直したときは、曲選択を出さずに、前回の曲でそのまま始める
	// (印は、読むと消える。その次に開いたときは、ふつうに曲選択が出る)
	if (IsLocalPlayerController() && IsRetryStart())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;

		// MusicPlayer / NoteSpawner の BeginPlay が終わってから始めるため、次のフレームに回す
		GetWorldTimerManager().SetTimerForNextTick(this, &ARhythmPlayerController::StartRetrySong);
		return;
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

		// 選んだ曲の番号を覚えておく(リトライで開き直したときに、同じ曲で始めるため)
		if (MusicPlayer)
		{
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UGameSessionSubsystem* Session = GI->GetSubsystem<UGameSessionSubsystem>())
				{
					Session->SetLastSongIndex(MusicPlayer->GetCurrentSongIndex());
				}
			}
		}

		// タイトルで止めておいた背景動画を再生する
		if (BackVideo) { BackVideo->ReleaseFromTitle(); }

		// 選んだ曲の譜面を読み込んで、開始する(失敗したときは、ログを出して、何もせず戻る)
		StartSelectedSong();
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	SongSelectWidget = nullptr;
}

// 選ばれている曲の譜面を読み込んで、譜面と音楽を開始する
// 成功したら true / 失敗したら、ログを出して false
bool ARhythmPlayerController::StartSelectedSong()
{
	// 必要なオブジェクトを確認
	if (!MusicPlayer || !NoteSpawner)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[PlayerController] MusicPlayer or NoteSpawner is null")
		);

		return false;
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

		return false;
	}

	// 選択中の難易度を取得
	FString SelectedChartPath;

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const URhythmSettingsSubsystem* Settings =
			GameInstance->GetSubsystem<URhythmSettingsSubsystem>())
		{
			switch (Settings->GetDifficulty())
			{
			case ERhythmDifficulty::VeryEasy:
				SelectedChartPath = SongData->VeryEasyChartFilePath;
				break;

			case ERhythmDifficulty::Easy:
				SelectedChartPath = SongData->EasyChartFilePath;
				break;

			case ERhythmDifficulty::Normal:
				SelectedChartPath = SongData->NormalChartFilePath;
				break;

			case ERhythmDifficulty::Hard:
				SelectedChartPath = SongData->HardChartFilePath;
				break;

			case ERhythmDifficulty::VeryHard:
				SelectedChartPath = SongData->VeryHardChartFilePath;
				break;

			default:
				SelectedChartPath = SongData->NormalChartFilePath;
				break;
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

		return false;
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

		return false;
	}

	// 譜面の読み込み成功後に開始する
	NoteSpawner->StartChart();
	MusicPlayer->PlayMusic();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[PlayerController] Chart and music started")
	);

	return true;
}

// リトライで開き直したときに、前回の曲で、曲選択を飛ばして始める
void ARhythmPlayerController::StartRetrySong()
{
	if (!MusicPlayer || !NoteSpawner)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerController] MusicPlayer or NoteSpawner is null (retry)"));
		return;
	}

	// 前回の曲を選び直す(難易度は URhythmSettingsSubsystem に残っている)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGameSessionSubsystem* Session = GI->GetSubsystem<UGameSessionSubsystem>())
		{
			const int32 LastIndex = Session->GetLastSongIndex();
			if (LastIndex != INDEX_NONE)
			{
				MusicPlayer->SelectSongByIndex(LastIndex);
			}
		}
	}

	StartSelectedSong();
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


	// 判定調整の間は、背景動画を止めておく
	if (BackVideo) { BackVideo->PauseVideos(); }

	// 曲選択が閉じていても時間が動かないよう、判定調整の間も時間を止めておく
	// (すでに止まっているときは、何も変わらない)
	SetPause(true);

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

	// マウスカーソルは出さず、UIはパッド・キーボードで操作する
	bShowMouseCursor = false;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	SetPause(true);

	// 背景動画を止める
	if (BackVideo) { BackVideo->PauseVideos(); }

	// 開いた直後は、押しっぱなしのキーで再開しないよう、少しの間ポーズ入力を無視する
	PauseInputIgnoreUntil = FPlatformTime::Seconds() + 0.3;

	bIsPaused = true;
}

void ARhythmPlayerController::ResumeGame()
{
	if (!bIsPaused) return;

	if (PauseWidget) {
		PauseWidget->RemoveFromParent();
	}

	// ゲーム入力に戻す
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	SetPause(false);

	// 背景動画を再開する
	if (BackVideo) { BackVideo->ResumeVideos(); }

	// 閉じた直後に、押しっぱなしでまた開かないようにする
	PauseInputIgnoreUntil = FPlatformTime::Seconds() + 0.3;

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