// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BackVideoActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMediaPlayer;
class UMediaSource;
class UMediaTexture;

USTRUCT(BlueprintType)
struct FBackVideoSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMediaPlayer> Player;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMediaSource> Source;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMediaTexture> Texture;
};

UCLASS()
class MUSICGAME_API ABackVideoActor : public AActor
{
	GENERATED_BODY()

public:
	ABackVideoActor();

	// 板。Detailsの Rotation / Location / Scale で、角度や位置を数値で調整できる
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UStaticMeshComponent> Video1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UStaticMeshComponent> Video2;

	// 3D用のマテリアル（M_VideoWorld）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	FBackVideoSlot Slot1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	FBackVideoSlot Slot2;

protected:
	virtual void BeginPlay() override;

private:
	void SetupSlot(UStaticMeshComponent* Mesh, const FBackVideoSlot& InSlot);
};