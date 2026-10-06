#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NoteData.h"
#include "MusicGameMode.generated.h"

class UAudioComponent;
class USoundBase;
class ANoteManager;

UCLASS()
class MUSICGAME_API AMusicGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AMusicGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	USoundBase* MusicSound;

private:

	UPROPERTY()
	TObjectPtr<ANoteManager> NoteManager;

	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicAudioComponent;

	TArray<FNoteData> Notes;

	float StartTime = 0.0f;
};