
#include "RhythmMusicPlayer.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "SongData.h"

ARhythmMusicPlayer::ARhythmMusicPlayer()
{
    PrimaryActorTick.bCanEverTick = false;

    AudioComponent =
        CreateDefaultSubobject<UAudioComponent>(
            TEXT("MusicAudioComponent")
        );

    SetRootComponent(AudioComponent);
    AudioComponent->bAutoActivate = false;
}

void ARhythmMusicPlayer::PlaySongByIndex(int32 Index)
{
    if (!Songs.IsValidIndex(Index) || !Songs[Index])
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("RhythmMusicPlayer: Invalid song index: %d"),
            Index
        );
        return;
    }

    CurrentSongData = Songs[Index];
    PlayMusic();
}

void ARhythmMusicPlayer::PlayMusic()
{
    if (!AudioComponent || !CurrentSongData)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("RhythmMusicPlayer: No song selected.")
        );
        return;
    }

    USoundBase* Sound = CurrentSongData->MusicSound;

    if (!Sound)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("RhythmMusicPlayer: MusicSound is not assigned.")
        );
        return;
    }

    AudioComponent->Stop();
    AudioComponent->SetSound(Sound);
    AudioComponent->Play();

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Now Playing: %s"),
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

void ARhythmMusicPlayer::BeginPlay()
{
    Super::BeginPlay();

    // 1‹È–Ú‚ğÄ¶‚µ‚ÄƒeƒXƒg
    PlaySongByIndex(0);
}