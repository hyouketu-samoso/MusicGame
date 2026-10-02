#include "RhythmPlayerController.h"
#include "RhythmInputComponent.h"

ARhythmPlayerController::ARhythmPlayerController()
{
	RhythmInput = 
		CreateDefaultSubobject<URhythmInputComponent>(TEXT("RhythmInput"));
}