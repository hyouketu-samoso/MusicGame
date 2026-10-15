#include "RhythmSettingsSubsystem.h"
#include "RhythmGameSettings.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// セーブスロット名（他のセーブデータと分けておく）
	const TCHAR* SettingsSlotName = TEXT("RhythmSettings");
	constexpr int32 SettingsUserIndex = 0;
}

void URhythmSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// まず初期値（速度3）、セーブがあればそちらで上書き
	NoteSpeedLevel = GetDefault<URhythmGameSettings>()->DefaultSpeedLevel;
	LoadSettings();
}

void URhythmSettingsSubsystem::SetNoteSpeedLevel(int32 NewLevel)
{
	const int32 Clamped = FMath::Clamp(NewLevel, GetMinNoteSpeedLevel(), GetMaxNoteSpeedLevel());
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
	SetNoteSpeedLevel(GetDefault<URhythmGameSettings>()->DefaultSpeedLevel);
}

float URhythmSettingsSubsystem::GetNoteArrivalTime() const
{
	return GetDefault<URhythmGameSettings>()->GetArrivalTime(NoteSpeedLevel);
}

int32 URhythmSettingsSubsystem::GetMinNoteSpeedLevel() const
{
	return URhythmGameSettings::MinSpeedLevel;
}

int32 URhythmSettingsSubsystem::GetMaxNoteSpeedLevel() const
{
	return URhythmGameSettings::MaxSpeedLevel;
}

void URhythmSettingsSubsystem::SetDifficulty(ERhythmDifficulty NewDifficulty)
{
	if (NewDifficulty == Difficulty)
	{
		return;
	}

	// 難易度を変えてもノーツ速度は変えない（仕様書）
	Difficulty = NewDifficulty;
	OnDifficultyChanged.Broadcast(Difficulty);
}

void URhythmSettingsSubsystem::SetInputOffsetMs(int32 NewOffsetMs)
{
	const int32 Clamped = FMath::Clamp(NewOffsetMs, -MaxInputOffsetMs, MaxInputOffsetMs);
	if (Clamped == InputOffsetMs)
	{
		return;
	}

	InputOffsetMs = Clamped;
	SaveSettings();
	OnInputOffsetChanged.Broadcast(InputOffsetMs);
}

void URhythmSettingsSubsystem::SaveSettings() const
{
	URhythmPlayerSettingsSaveGame* Save = Cast<URhythmPlayerSettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(URhythmPlayerSettingsSaveGame::StaticClass()));
	if (Save)
	{
		Save->NoteSpeedLevel = NoteSpeedLevel;
		Save->InputOffsetMs = InputOffsetMs;
		UGameplayStatics::SaveGameToSlot(Save, SettingsSlotName, SettingsUserIndex);
	}
}

void URhythmSettingsSubsystem::LoadSettings()
{
	if (!UGameplayStatics::DoesSaveGameExist(SettingsSlotName, SettingsUserIndex))
	{
		return;
	}

	const URhythmPlayerSettingsSaveGame* Save = Cast<URhythmPlayerSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, SettingsUserIndex));
	if (Save && Save->NoteSpeedLevel > 0)
	{
		NoteSpeedLevel = FMath::Clamp(Save->NoteSpeedLevel, GetMinNoteSpeedLevel(), GetMaxNoteSpeedLevel());
	}

	// 古いセーブデータには無いので、その場合は0のまま
	if (Save)
	{
		InputOffsetMs = FMath::Clamp(Save->InputOffsetMs, -MaxInputOffsetMs, MaxInputOffsetMs);
	}
}
