#include "RhythmSettingsSubsystem.h"
#include "RhythmGameSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Input/RhythmSettingsSaveGame.h"

namespace
{
    // セーブスロット名
    const TCHAR* SettingsSlotName = TEXT("RhythmSettings");
    constexpr int32 SettingsUserIndex = 0;
}

void URhythmSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 初期値を設定してから保存データを読み込む
    NoteSpeedLevel =
        GetDefault<URhythmGameSettings>()->DefaultSpeedLevel;

    LoadSettings();
}

void URhythmSettingsSubsystem::SetNoteSpeedLevel(int32 NewLevel)
{
    const int32 Clamped = FMath::Clamp(NewLevel,GetMinNoteSpeedLevel(),GetMaxNoteSpeedLevel());

    if (Clamped == NoteSpeedLevel)
    {
        return;
    }

    NoteSpeedLevel = Clamped;
    SaveSettings();
    OnNoteSpeedLevelChanged.Broadcast(NoteSpeedLevel);
}

void URhythmSettingsSubsystem::ResetNoteSpeedLevel()
{
    SetNoteSpeedLevel(
        GetDefault<URhythmGameSettings>()->DefaultSpeedLevel);
}

float URhythmSettingsSubsystem::GetNoteArrivalTime() const
{
    return GetDefault<URhythmGameSettings>()
        ->GetArrivalTime(NoteSpeedLevel);
}

int32 URhythmSettingsSubsystem::GetMinNoteSpeedLevel() const
{
    return URhythmGameSettings::MinSpeedLevel;
}

int32 URhythmSettingsSubsystem::GetMaxNoteSpeedLevel() const
{
    return URhythmGameSettings::MaxSpeedLevel;
}

// 難易度設定
void URhythmSettingsSubsystem::SetDifficulty(ERhythmDifficulty NewDifficulty)
{
    if (NewDifficulty == Difficulty)
    {
        return;
    }

    // 難易度を変更してもノーツ速度は変えない
    Difficulty = NewDifficulty;

    SaveSettings();
    OnDifficultyChanged.Broadcast(Difficulty);
}

// 入力タイミング補正
void URhythmSettingsSubsystem::SetInputOffsetMs(int32 NewOffsetMs)
{
    const int32 Clamped = FMath::Clamp(NewOffsetMs,-MaxInputOffsetMs,MaxInputOffsetMs);

    if (Clamped == InputOffsetMs)
    {
        return;
    }

    InputOffsetMs = Clamped;
    SaveSettings();
    OnInputOffsetChanged.Broadcast(InputOffsetMs);
}

// デッドゾーン
float URhythmSettingsSubsystem::GetDeadZone() const
{
    return DeadZone;
}

void URhythmSettingsSubsystem::SetDeadZone(float NewDeadZone)
{
    const float Clamped = FMath::Clamp(NewDeadZone, 0.0f, 0.99f);

    if (FMath::IsNearlyEqual(Clamped, DeadZone))
    {
        return;
    }

    DeadZone = Clamped;
    SaveSettings();
}

// 入力感度
int32 URhythmSettingsSubsystem::GetSensitivityLevel() const
{
    return SensitivityLevel;
}

void URhythmSettingsSubsystem::SetSensitivityLevel(int32 NewLevel)
{
    const int32 Clamped = FMath::Clamp(NewLevel, 1, 5);

    if (Clamped == SensitivityLevel)
    {
        return;
    }

    SensitivityLevel = Clamped;
    SaveSettings();
}

// 振動設定
bool URhythmSettingsSubsystem::GetVibrationEnabled() const
{
    return bVibration;
}

void URhythmSettingsSubsystem::SetVibrationEnabled(bool bEnabled)
{
    if (bVibration == bEnabled)
    {
        return;
    }

    bVibration = bEnabled;
    SaveSettings();
}

// マスター音量
int32 URhythmSettingsSubsystem::GetMasterVolumeLevel() const
{
    return MasterVolumeLevel;
}

void URhythmSettingsSubsystem::SetMasterVolumeLevel(int32 NewLevel)
{
    const int32 Clamped = FMath::Clamp(NewLevel, 0, 100);

    if (Clamped == MasterVolumeLevel)
    {
        return;
    }

    MasterVolumeLevel = Clamped;
    SaveSettings();
}

// BGM音量
int32 URhythmSettingsSubsystem::GetBGMVolumeLevel() const
{
    return BGMVolumeLevel;
}

void URhythmSettingsSubsystem::SetBGMVolumeLevel(int32 NewLevel)
{
    const int32 Clamped = FMath::Clamp(NewLevel, 0, 100);

    if (Clamped == BGMVolumeLevel)
    {
        return;
    }

    BGMVolumeLevel = Clamped;
    SaveSettings();
}

// SE音量
int32 URhythmSettingsSubsystem::GetSEVolumeLevel() const
{
    return SEVolumeLevel;
}

void URhythmSettingsSubsystem::SetSEVolumeLevel(int32 NewLevel)
{
    const int32 Clamped = FMath::Clamp(NewLevel, 0, 100);

    if (Clamped == SEVolumeLevel)
    {
        return;
    }

    SEVolumeLevel = Clamped;
    SaveSettings();
}

// 背景の明るさ
int32 URhythmSettingsSubsystem::GetBGBrightnessStep() const
{
    return BGBrightnessStep;
}

void URhythmSettingsSubsystem::SetBGBrightnessStep(int32 NewStep)
{
    const int32 Clamped = FMath::Clamp(NewStep, 0, 10);

    if (Clamped == BGBrightnessStep)
    {
        return;
    }

    BGBrightnessStep = Clamped;
    SaveSettings();
}

// レーンの明るさ
int32 URhythmSettingsSubsystem::GetLaneBrightnessStep() const
{
    return LaneBrightnessStep;
}

void URhythmSettingsSubsystem::SetLaneBrightnessStep(int32 NewStep)
{
    const int32 Clamped = FMath::Clamp(NewStep, 0, 10);

    if (Clamped == LaneBrightnessStep)
    {
        return;
    }

    LaneBrightnessStep = Clamped;
    SaveSettings();
}

// ノーツの明るさ
int32 URhythmSettingsSubsystem::GetNoteBrightnessStep() const
{
    return NoteBrightnessStep;
}

// 曲名と現在の難易度から保存キーを作成
namespace
{
    FString MakeBestScoreKey(
        const FString& SongTitle,
        ERhythmDifficulty Difficulty)
    {
        FString DifficultyName;

        switch (Difficulty)
        {
        case ERhythmDifficulty::Easyst:
            DifficultyName = TEXT("Easyst");
            break;

        case ERhythmDifficulty::Easy:
            DifficultyName = TEXT("Easy");
            break;

        case ERhythmDifficulty::Normal:
            DifficultyName = TEXT("Normal");
            break;

        case ERhythmDifficulty::Hard:
            DifficultyName = TEXT("Hard");
            break;

        case ERhythmDifficulty::Hardest:
            DifficultyName = TEXT("Hardest");
            break;

        default:
            DifficultyName = TEXT("Normal");
            break;
        }

        return FString::Printf(
            TEXT("%s|%s"),
            *SongTitle,
            *DifficultyName);
    }
}
// 最高スコアを取得
int64 URhythmSettingsSubsystem::GetBestScore(
    FString SongTitle) const
{
    const FString Key = MakeBestScoreKey(SongTitle, Difficulty);

    if (const int64* Score = BestScores.Find(Key))
    {
        return *Score;
    }

    // 未記録の場合
    return 0;
}

// 最高スコアを更新
bool URhythmSettingsSubsystem::TryUpdateBestScore(
    FString SongTitle,
    int64 NewScore)
{
    if (SongTitle.IsEmpty() || NewScore < 0)
    {
        return false;
    }

    const FString Key = MakeBestScoreKey(SongTitle, Difficulty);
    const int64* PreviousScore = BestScores.Find(Key);

    // 既存記録以上なら更新しない
    if (PreviousScore && NewScore <= *PreviousScore)
    {
        return false;
    }

    BestScores.Add(Key, NewScore);

    // 既存の設定保存処理を利用してまとめて保存
    SaveSettings();

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[RhythmRecord] New record: %s = %lld"),
        *Key,
        NewScore);

    return true;
}

void URhythmSettingsSubsystem::SetNoteBrightnessStep(int32 NewStep)
{
    const int32 Clamped = FMath::Clamp(NewStep, 0, 10);

    if (Clamped == NoteBrightnessStep)
    {
        return;
    }

    NoteBrightnessStep = Clamped;
    SaveSettings();
}

// 全設定を保存
void URhythmSettingsSubsystem::SaveSettings() const
{
    URhythmSettingsSaveGame* Save =Cast<URhythmSettingsSaveGame>
        (UGameplayStatics::CreateSaveGameObject(URhythmSettingsSaveGame::StaticClass()));

    if (!Save)
    {
        return;
    }

    // リズム設定
    Save->NoteSpeedLevel = NoteSpeedLevel;
    Save->InputOffsetMs = InputOffsetMs;
    Save->Difficulty = Difficulty;

    // 入力設定
    Save->DeadZone = DeadZone;
    Save->SensitivityLevel = SensitivityLevel;
    Save->bVibration = bVibration;

    // 音量設定
    Save->MasterVolumeLevel = MasterVolumeLevel;
    Save->BGMVolumeLevel = BGMVolumeLevel;
    Save->SEVolumeLevel = SEVolumeLevel;

    // 明るさ設定
    Save->BGBrightnessStep = BGBrightnessStep;
    Save->LaneBrightnessStep = LaneBrightnessStep;
    Save->NoteBrightnessStep = NoteBrightnessStep;

    // 最高スコア
    Save->BestScores = BestScores;

    const bool bSaved = UGameplayStatics::SaveGameToSlot(
        Save,
        SettingsSlotName,
        SettingsUserIndex);

    if (!bSaved)
    {
        UE_LOG(LogTemp, Warning,TEXT("[Settings] Failed to save settings."));
    }
}

// 全設定を読み込み
void URhythmSettingsSubsystem::LoadSettings()
{
    if (!UGameplayStatics::DoesSaveGameExist(SettingsSlotName,SettingsUserIndex))
    {
        // セーブがなければ初期値を使用
        return;
    }

    URhythmSettingsSaveGame* Save =Cast<URhythmSettingsSaveGame>
        (UGameplayStatics::LoadGameFromSlot(SettingsSlotName,SettingsUserIndex));

    if (!Save)
    {
        UE_LOG(LogTemp, Warning,TEXT("[Settings] Failed to load settings."));
        return;
    }

    // リズム設定
    NoteSpeedLevel = FMath::Clamp(Save->NoteSpeedLevel,GetMinNoteSpeedLevel(),GetMaxNoteSpeedLevel());

    InputOffsetMs = FMath::Clamp(Save->InputOffsetMs,-MaxInputOffsetMs,MaxInputOffsetMs);

    Difficulty = Save->Difficulty;

    // 入力設定
    DeadZone = FMath::Clamp(Save->DeadZone, 0.0f, 0.99f);

    SensitivityLevel = FMath::Clamp(Save->SensitivityLevel, 1, 5);

    bVibration = Save->bVibration;

    // 音量設定
    MasterVolumeLevel = FMath::Clamp(Save->MasterVolumeLevel, 0, 100);

    BGMVolumeLevel = FMath::Clamp(Save->BGMVolumeLevel, 0, 100);

    SEVolumeLevel = FMath::Clamp(Save->SEVolumeLevel, 0, 100);

    // 明るさ設定
    BGBrightnessStep = FMath::Clamp(Save->BGBrightnessStep, 0, 10);

    LaneBrightnessStep = FMath::Clamp(Save->LaneBrightnessStep, 0, 10);

    NoteBrightnessStep = FMath::Clamp(Save->NoteBrightnessStep, 0, 10);

    // 最高スコア
    BestScores = Save->BestScores;

    UE_LOG(LogTemp, Log,TEXT("[Settings] Settings loaded successfully."));
}