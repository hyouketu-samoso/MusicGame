#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResultNiagaraWidgetBase.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

// Niagara の終了通知
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResultNiagaraFinished);

UCLASS(Abstract)
class MUSICGAME_API UResultNiagaraWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    // Niagara が終了したときに通知する
    UPROPERTY(BlueprintAssignable, Category = "Result Effect")
    FOnResultNiagaraFinished OnNiagaraFinished;

protected:
    // ウィジェットごとに設定する Niagara
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result Effect")
    TObjectPtr<UNiagaraSystem> NiagaraEffect = nullptr;

    // カメラからエフェクトまでの距離（cm）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result Effect",
        meta = (ClampMin = "50.0"))
    float SpawnDistance = 300.0f;

    // 再生中の Niagara コンポーネント
    UPROPERTY(Transient)
    TObjectPtr<UNiagaraComponent> ActiveNiagaraComponent = nullptr;

    // 終了通知の二重送信を防ぐ
    bool bFinishNotificationSent = false;

    // Niagara を再生する
    UFUNCTION(BlueprintCallable, Category = "Result Effect")
    void PlayNiagaraEffect();

    // Niagara の再生終了時に呼ばれる
    UFUNCTION()
    void HandleNiagaraSystemFinished(UNiagaraComponent* FinishedComponent);

    // Niagara が再生できなかった場合の終了通知
    UFUNCTION()
    void NotifyNiagaraFinished();
};