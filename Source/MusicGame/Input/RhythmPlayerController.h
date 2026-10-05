// ============================================================
// RhythmPlayerController
//  役割 : RhythmInputComponent を持ち、ポーズの開閉を管理する
//	ポーズ : Esc /Start -> OnPauseInput -> TogglePause
// ============================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RhythmPlayerController.generated.h"

class URhythmInputComponent;
class UUserWidget;

UCLASS()
class MUSICGAME_API ARhythmPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ARhythmPlayerController();

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	TObjectPtr<URhythmInputComponent>RhythmInput;

	// ポーズ画面のWidget(BPで WBP_PauseMenu を指定する)
	UPROPERTY(EditAnyWhere, BluePrintReadOnly,Category = "Rhythm|Pause")
	TSubclassOf<UUserWidget>PauseWidgetClass;

	//　ポーズの開閉を切り替える
	UFUNCTION(BluePrintCallable,Category = "Rhythm|Pause")
	void TogglePause();

	// ポーズを解除する(再開ボタンにも使用可)
	UFUNCTION(BluePrintCallable, Category = "Rhythm|Pause")
	void ResumeGame();

	// ゲームを終了する(強制終了ボタン)
	UFUNCTION(BluePrintCallable, Category = "Rhythm|Pause")
	void QuitToWindows();

protected:
	virtual void BeginPlay() override;

private:
	void OpenPause();

	UFUNCTION()
	void HandlePauseInput();

	UPROPERTY()
	TObjectPtr<UUserWidget>PauseWidget;

	bool bIsPaused = false;
};
