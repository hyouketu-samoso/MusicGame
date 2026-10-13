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
};