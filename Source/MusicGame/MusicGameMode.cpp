#include "MusicGameMode.h"
#include "ChartImporter.h"
#include "NoteManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"

void AMusicGameMode::BeginPlay()
{
    Super::BeginPlay();

    // JSON 読み込み
    UChartImporter* Importer = NewObject<UChartImporter>();
    Importer->LoadChart(FPaths::ProjectContentDir() / TEXT("Chart/chart.json"), Notes);

    // NoteManager 生成
    NoteManager = GetWorld()->SpawnActor<ANoteManager>();
    NoteManager->InitNotes(Notes);

    PrimaryActorTick.bCanEverTick = true;

    // 曲の再生
    MusicAudioComponent = UGameplayStatics::SpawnSoundAttached(
        MusicSound,
        GetRootComponent()
    );

    // ★ 曲の再生開始時間を記録（Quartz の代わり）
    StartTime = UGameplayStatics::GetTimeSeconds(GetWorld());
}

void AMusicGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!NoteManager || !MusicAudioComponent) return;

    // ★ 現在の曲の経過時間（Quartz の代わり）
    const float Now = UGameplayStatics::GetTimeSeconds(GetWorld());
    const float CurrentTime = Now - StartTime;

    // ノーツ生成
    NoteManager->UpdateSpawn(CurrentTime, SpawnOffset);
}
