#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RhythmSettingsSaveGame.generated.h"

UCLASS()
class MUSICGAME_API URhythmSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() float DeadZone = 0.20f;
	UPROPERTY() int32 SensitivityLevel = 3;
	UPROPERTY() bool bVibration = true;
	UPROPERTY() int32 BGMVolumeLevel = 100;
	UPROPERTY() int32 SEVolumeLevel = 100;
	UPROPERTY() int32 MasterVolumeLevel = 100;
	// 明るさ(0〜10の段階。5が標準)。古い保存データには無いので、読み込み時は5になる
	UPROPERTY() int32 BGBrightnessStep = 5;
	UPROPERTY() int32 LaneBrightnessStep = 5;
	UPROPERTY() int32 NoteBrightnessStep = 5;
};