#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "SongData.generated.h"

UCLASS(BlueprintType)
class MUSICGAME_API USongData : public UDataAsset
{
    GENERATED_BODY()

public:
    // 曲名
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song")
    FText SongTitle;

    // BGM
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song")
    TObjectPtr<USoundBase> MusicSound;

    // NORMAL 譜面の JSON パス
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song|Chart")
    FString NormalChartFilePath;

    // HARD 譜面の JSON パス
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song|Chart")
    FString HardChartFilePath;
};