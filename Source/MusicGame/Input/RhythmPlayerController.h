#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RhythmPlayerController.generated.h"

class URhythmInputComponent;

UCLASS()
class MUSICGAME_API ARhythmPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ARhythmPlayerController();

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	TObjectPtr<URhythmInputComponent>RhythmInput;
};
