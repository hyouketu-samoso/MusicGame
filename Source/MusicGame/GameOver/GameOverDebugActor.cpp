#include "GameOverDebugActor.h"

#include "GameOverWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

AGameOverDebugActor::AGameOverDebugActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGameOverDebugActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// すでに出しているときは、何もしない
	if (Widget && Widget->IsInViewport())
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !GameOverWidgetClass)
	{
		return;
	}

	// 指定のキーが、このフレームで押されたら、画面を作って出す
	if (PC->WasInputKeyJustPressed(TriggerKey))
	{
		Widget = CreateWidget<UGameOverWidget>(PC, GameOverWidgetClass);
		if (Widget)
		{
			Widget->Open();
		}
	}
}