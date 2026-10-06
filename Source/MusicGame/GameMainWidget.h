// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMainWidget.generated.h"

class UMediaPlayer;
class UMediaSource;

/**
 * 
 */
UCLASS()
class UGameMainWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Video")
	void PlayVideos();

	UFUNCTION(BlueprintCallable, Category = "Video")
	void StopVideos();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
		TObjectPtr<UMediaPlayer> Player1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
		TObjectPtr<UMediaSource> Source1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
		TObjectPtr<UMediaPlayer> Player2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
		TObjectPtr<UMediaSource> Source2;

};
