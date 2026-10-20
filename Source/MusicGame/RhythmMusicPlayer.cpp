
#include "RhythmMusicPlayer.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "SongData.h"

ARhythmMusicPlayer::ARhythmMusicPlayer()
{
    PrimaryActorTick.bCanEverTick = false;

    AudioComponent = CreateDefaultSubobject<UAudioComponent>(
        TEXT("MusicAudioComponent")
    );

    SetRootComponent(AudioComponent);
    AudioComponent->bAutoActivate = false;
}

void ARhythmMusicPlayer::BeginPlay()
{
    Super::BeginPlay();

    if (Songs.IsValidIndex(0) && Songs[0])
    {
        CurrentSongIndex = 0;
        CurrentSongData = Songs[0];

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[MusicPlayer] Initial song: %s"),
            *CurrentSongData->SongTitle.ToString()
        );
    }
    else
    {
        CurrentSongIndex = INDEX_NONE;
        CurrentSongData = nullptr;

        UE_LOG(
            LogTemp,
            Error,
            TEXT("[MusicPlayer] No song data is configured in Songs")
        );
    }
}

void ARhythmMusicPlayer::PlaySongByIndex(int32 Index)
{
    if (!Songs.IsValidIndex(Index) || !Songs[Index])
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[MusicPlayer] Invalid song index: %d"),
            Index
        );
        return;
    }

    CurrentSongIndex = Index;
    CurrentSongData = Songs[Index];

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[MusicPlayer] Song selected: Index=%d, Title=%s"),
        CurrentSongIndex,
        *CurrentSongData->SongTitle.ToString()
    );

    PlayMusic();
}

void ARhythmMusicPlayer::SelectSongByIndex(int32 Index)
{
    if (!Songs.IsValidIndex(Index) || !Songs[Index])
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[MusicPlayer] Invalid song index: %d"),
            Index
        );
        return;
    }

    CurrentSongIndex = Index;
    CurrentSongData = Songs[Index];

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[MusicPlayer] Selected: Index=%d, Title=%s"),
        CurrentSongIndex,
        *CurrentSongData->SongTitle.ToString()
    );
}

void ARhythmMusicPlayer::PlayMusic()
{
    if (!AudioComponent || !CurrentSongData)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[MusicPlayer] Cannot play music. No song is selected")
        );
        return;
    }

    USoundBase* Sound = CurrentSongData->MusicSound;

    if (!Sound)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[MusicPlayer] MusicSound is not set: %s"),
            *CurrentSongData->SongTitle.ToString()
        );
        return;
    }

    AudioComponent->Stop();
    AudioComponent->SetSound(Sound);
    AudioComponent->Play();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[MusicPlayer] Now Playing: %s"),
        *CurrentSongData->SongTitle.ToString()
    );
}

void ARhythmMusicPlayer::StopMusic()
{
    if (AudioComponent)
    {
        AudioComponent->Stop();
    }
}

void ARhythmMusicPlayer::PauseMusic()
{
    if (AudioComponent)
    {
        AudioComponent->SetPaused(true);
    }
}

void ARhythmMusicPlayer::ResumeMusic()
{
    if (AudioComponent)
    {
        AudioComponent->SetPaused(false);
    }
}

USongData* ARhythmMusicPlayer::GetCurrentSongData() const
{
    return CurrentSongData;
}

void ARhythmMusicPlayer::HandleSongChangeRequested(int32 Direction)
{
    const int32 SongCount = Songs.Num();

    if (SongCount <= 0 || Direction == 0)
    {
        return;
    }

    if (!Songs.IsValidIndex(CurrentSongIndex))
    {
        PlaySongByIndex(0);
        return;
    }

    const int32 NextIndex =
        (CurrentSongIndex + Direction + SongCount) % SongCount;

  SelectSongByIndex(NextIndex);
}

int32 ARhythmMusicPlayer::GetSongCount() const
{
    return Songs.Num();
}

USongData* ARhythmMusicPlayer::GetSongDataByIndex(int32 Index) const
{
    return Songs.IsValidIndex(Index) ? Songs[Index] : nullptr;
}

int32 ARhythmMusicPlayer::GetCurrentSongIndex() const
{
    return CurrentSongIndex;
}
