#include "ResultNiagaraWidgetBase.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UResultNiagaraWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    bFinishNotificationSent = false;

    // ウィジェットが構築されたタイミングで再生
    PlayNiagaraEffect();
}

void UResultNiagaraWidgetBase::PlayNiagaraEffect()
{
    if (!NiagaraEffect)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s: NiagaraEffect is not assigned"),
            *GetName()
        );

        NotifyNiagaraFinished();
        return;
    }

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC || !PC->PlayerCameraManager)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s: PlayerCameraManager is not available"),
            *GetName()
        );

        NotifyNiagaraFinished();
        return;
    }

    const FVector CameraLocation =
        PC->PlayerCameraManager->GetCameraLocation();

    const FRotator CameraRotation =
        PC->PlayerCameraManager->GetCameraRotation();

    const FVector SpawnLocation =
        CameraLocation + CameraRotation.Vector() * SpawnDistance;

    // 最初は自動再生を無効にし、終了イベントを登録してから再生する
    ActiveNiagaraComponent =
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            NiagaraEffect,
            SpawnLocation,
            CameraRotation,
            FVector(1.0f),
            true,   // 終了時に自動破棄
            false   // イベント登録前には自動再生しない
        );

    if (!ActiveNiagaraComponent)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s: Failed to spawn Niagara"),
            *GetName()
        );

        NotifyNiagaraFinished();
        return;
    }

    // Niagara の終了イベントを登録
    ActiveNiagaraComponent->OnSystemFinished.AddUniqueDynamic(
        this,
        &UResultNiagaraWidgetBase::HandleNiagaraSystemFinished
    );

    // イベント登録後に再生する
    ActiveNiagaraComponent->Activate(true);
}

void UResultNiagaraWidgetBase::HandleNiagaraSystemFinished(
    UNiagaraComponent* FinishedComponent)
{
    if (FinishedComponent != ActiveNiagaraComponent)
    {
        return;
    }

    // 終了した Niagara のイベント登録を解除
    FinishedComponent->OnSystemFinished.RemoveDynamic(
        this,
        &UResultNiagaraWidgetBase::HandleNiagaraSystemFinished
    );

    ActiveNiagaraComponent = nullptr;

    NotifyNiagaraFinished();
}

void UResultNiagaraWidgetBase::NotifyNiagaraFinished()
{
    if (bFinishNotificationSent)
    {
        return;
    }

    bFinishNotificationSent = true;

    // RhythmGameFlowComponent に終了を通知
    OnNiagaraFinished.Broadcast();
}