#include "RhythmGameFlowComponent.h"
#include "RhythmJudge.h"
#include "RhythmScoreComponent.h"
#include "RhythmSettingsSubsystem.h"
#include "NoteSpawner.h"
#include "RhythmMusicPlayer.h"
#include "SongData.h"
#include "Blueprint/UserWidget.h"
#include "GameOver/GameOverWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

URhythmGameFlowComponent::URhythmGameFlowComponent()
{
	// 曲の終わりを毎フレーム見る（ポーズ中は止まる）
	PrimaryComponentTick.bCanEverTick = true;

	// リザルト画面は渋谷さんの WBP_Result を初期値にする（詳細パネルで差し替えられる）
	static ConstructorHelpers::FClassFinder<URhythmResultWidget> ResultWidgetFinder(TEXT("/Game/MusicGame/Widget_P/Widget_Shibuya/WBP_Result"));
	if (ResultWidgetFinder.Succeeded())
	{
		ResultWidgetClass = ResultWidgetFinder.Class;
	}

	// ゲームオーバー（リトライ）画面は渋谷さんの WBP_GameOver を初期値にする
	static ConstructorHelpers::FClassFinder<UUserWidget> GameOverWidgetFinder(TEXT("/Game/MusicGame/Widget_P/Widget_Shibuya/WBP_GameOver"));
	if (GameOverWidgetFinder.Succeeded())
	{
		RetryWidgetClass = GameOverWidgetFinder.Class;
	}
}

void URhythmGameFlowComponent::BindJudge(ARhythmJudge* InJudge)
{
	if (Judge == InJudge || !IsValid(InJudge))
	{
		return;
	}

	if (IsValid(Judge))
	{
		Judge->OnNoteJudged.RemoveDynamic(this, &URhythmGameFlowComponent::HandleNoteJudged);
		Judge->GetScoreComponent()->OnSongFinished.RemoveDynamic(this, &URhythmGameFlowComponent::HandleSongFinished);
		Judge->GetScoreComponent()->OnGameOver.RemoveDynamic(this, &URhythmGameFlowComponent::HandleGameOver);
	}

	Judge = InJudge;
	Judge->OnNoteJudged.AddDynamic(this, &URhythmGameFlowComponent::HandleNoteJudged);
	Judge->GetScoreComponent()->OnSongFinished.AddDynamic(this, &URhythmGameFlowComponent::HandleSongFinished);
	Judge->GetScoreComponent()->OnGameOver.AddDynamic(this, &URhythmGameFlowComponent::HandleGameOver);
}

void URhythmGameFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScreenTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void URhythmGameFlowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bScreenRequested)
	{
		return;
	}

	URhythmScoreComponent* Score = GetScore();
	if (!Score || Score->IsGameOver() || Score->IsSongFinished())
	{
		return;
	}

	if (IsSongEnded())
	{
		// クリア判定をして OnSongFinished を出す → HandleSongFinished でリザルトへ
		Score->FinishSong();
	}
}

bool URhythmGameFlowComponent::IsSongEnded() const
{
	const ANoteSpawner* Spawner = GetSpawner();
	const URhythmScoreComponent* Score = GetScore();
	if (!Spawner || !Score || !Spawner->IsChartRunning() || !Spawner->HasSpawnedAllChartNotes())
	{
		return false;
	}

	// 譜面の時間が曲の長さを過ぎたか
	if (Spawner->GetSongTime() < GetSongLength())
	{
		return false;
	}

	// 譜面の判定がすべて終わったか（総数が未設定なら時間だけで判断する）
	const int32 Total = Score->GetTotalJudgeCount();
	return Total <= 0 || Score->GetJudgedCount() >= Total;
}

float URhythmGameFlowComponent::GetSongLength() const
{
	// 長さが分からない音（ループ設定など）は使わない
	auto GetValidDuration = [](const USoundBase* Sound) -> float
	{
		if (!Sound)
		{
			return 0.0f;
		}
		const float Duration = Sound->GetDuration();
		return (Duration > 0.0f && Duration < INDEFINITELY_LOOPING_DURATION) ? Duration : 0.0f;
	};

	// 1. 手動で設定した BGM
	if (const float Duration = GetValidDuration(SongSoundOverride); Duration > 0.0f)
	{
		return Duration;
	}

	// 2. 選択中の曲の BGM
	if (const ARhythmMusicPlayer* MusicPlayer = FindMusicPlayer())
	{
		if (const USongData* SongData = MusicPlayer->GetCurrentSongData())
		{
			if (const float Duration = GetValidDuration(SongData->MusicSound); Duration > 0.0f)
			{
				return Duration;
			}
		}
	}

	// 3. 最後のノーツ + 余白
	const ANoteSpawner* Spawner = GetSpawner();
	return (Spawner ? Spawner->GetChartEndTime() : 0.0f) + EndMarginAfterLastNote;
}

void URhythmGameFlowComponent::HandleNoteJudged(ERhythmJudgement Judgement, int32 LaneIndex, float TimingError, int32 Combo, ENoteJudgePoint Point)
{
	// ミス（タイミングなし）と、設定によっては Perfect を除いて、早い / 遅いを数える
	if (Judgement == ERhythmJudgement::Miss || (Judgement == ERhythmJudgement::Perfect && !bCountPerfectInFastLate))
	{
		return;
	}

	if (TimingError < 0.0f)
	{
		++FastCount;
	}
	else if (TimingError > 0.0f)
	{
		++LateCount;
	}
}

void URhythmGameFlowComponent::HandleSongFinished(bool bCleared, double FinalScore)
{
	if (bScreenRequested)
	{
		return;
	}
	bScreenRequested = true;

	GetWorld()->GetTimerManager().SetTimer(ScreenTimer, this, &URhythmGameFlowComponent::ShowResultScreen, FMath::Max(ResultDelay, 0.01f), false);
}

void URhythmGameFlowComponent::HandleGameOver()
{
	if (bScreenRequested)
	{
		return;
	}
	bScreenRequested = true;

	if (bStopMusicOnGameOver)
	{
		if (ARhythmMusicPlayer* MusicPlayer = FindMusicPlayer())
		{
			MusicPlayer->StopMusic();
		}
	}

	GetWorld()->GetTimerManager().SetTimer(ScreenTimer, this, &URhythmGameFlowComponent::ShowRetryScreen, FMath::Max(RetryDelay, 0.01f), false);
}

void URhythmGameFlowComponent::ShowResultScreen()
{
	const FRhythmResultData Data = BuildResultData();
	OnResultScreenRequested.Broadcast(Data);

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !ResultWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("RhythmGameFlow: ResultWidgetClass が未設定のため、リザルト画面を出しません"));
		return;
	}

	URhythmResultWidget* Result = CreateWidget<URhythmResultWidget>(PC, ResultWidgetClass);
	if (!Result)
	{
		return;
	}

	// 入力の切り替え・ポーズ・リトライ / タイトルの移動はリザルト画面が自分で行う
	Result->SetResult(Data);
	Result->AddToViewport(1000);
	ShownWidget = Result;
}

void URhythmGameFlowComponent::ShowRetryScreen()
{
	OnRetryScreenRequested.Broadcast();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !RetryWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("RhythmGameFlow: RetryWidgetClass が未設定のため、リトライ画面を出しません（OnRetryScreenRequested のみ）"));
		return;
	}

	UUserWidget* Retry = CreateWidget<UUserWidget>(PC, RetryWidgetClass);
	if (!Retry)
	{
		return;
	}

	// ゲームオーバー画面は Open() で、画面に出す・ポーズ・BGM 停止・入力の切り替えまで自分で行う
	if (UGameOverWidget* GameOver = Cast<UGameOverWidget>(Retry))
	{
		GameOver->Open();
		ShownWidget = GameOver;
		return;
	}

	ShowWidget(Retry);

	if (bPauseWhileRetryShown)
	{
		PC->SetPause(true);
	}
}

void URhythmGameFlowComponent::ShowWidget(UUserWidget* Widget)
{
	Widget->AddToViewport(1000);
	ShownWidget = Widget;

	// ボタンをマウス・パッドで操作できるようにする
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(Widget->TakeWidget());
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
}

FRhythmResultData URhythmGameFlowComponent::BuildResultData() const
{
	FRhythmResultData Data;

	if (const URhythmScoreComponent* Score = GetScore())
	{
		Data.Score        = static_cast<float>(Score->GetDisplayScore());
		Data.MaxCombo     = Score->GetMaxCombo();
		Data.PerfectCount = Score->GetJudgementCount(ERhythmJudgement::Perfect);
		Data.GreatCount   = Score->GetJudgementCount(ERhythmJudgement::Great);
		Data.GoodCount    = Score->GetJudgementCount(ERhythmJudgement::Good);
		Data.BadCount     = Score->GetJudgementCount(ERhythmJudgement::Bad);
		Data.MissCount    = Score->GetJudgementCount(ERhythmJudgement::Miss);
	}

	Data.FastCount = FastCount;
	Data.LateCount = LateCount;

	// 曲名は選択中の曲から
	if (const ARhythmMusicPlayer* MusicPlayer = FindMusicPlayer())
	{
		if (const USongData* SongData = MusicPlayer->GetCurrentSongData())
		{
			Data.SongTitle = SongData->SongTitle;
		}
	}

	// 難易度は設定置き場から（並びが同じ Normal / Hard）
	if (const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this))
	{
		if (const URhythmSettingsSubsystem* Settings = GameInstance->GetSubsystem<URhythmSettingsSubsystem>())
		{
			Data.Difficulty = (Settings->GetDifficulty() == ERhythmDifficulty::Hard) ? ESongDifficulty::Hard : ESongDifficulty::Normal;
		}
	}

	// ランクはリザルト画面がスコアから決める（bAutoRankFromScore）。
	// FULL COMBO / ALL PERFECT・NEW RECORD は決め方が未定のため false のまま

	return Data;
}

ANoteSpawner* URhythmGameFlowComponent::GetSpawner() const
{
	return Cast<ANoteSpawner>(GetOwner());
}

URhythmScoreComponent* URhythmGameFlowComponent::GetScore() const
{
	return IsValid(Judge) ? Judge->GetScoreComponent() : nullptr;
}

ARhythmMusicPlayer* URhythmGameFlowComponent::FindMusicPlayer() const
{
	return Cast<ARhythmMusicPlayer>(UGameplayStatics::GetActorOfClass(this, ARhythmMusicPlayer::StaticClass()));
}
