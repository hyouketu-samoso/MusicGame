// Fill out your copyright notice in the Description page of Project Settings.


#include "BackVideoActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "UObject/ConstructorHelpers.h"

ABackVideoActor::ABackVideoActor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Video1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Video1"));
	Video1->SetupAttachment(Root);
	Video2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Video2"));
	Video2->SetupAttachment(Root);

	// エンジン標準の板（Plane）
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		Video1->SetStaticMesh(PlaneMesh.Object);
		Video2->SetStaticMesh(PlaneMesh.Object);
	}

	// 影や当たり判定は不要
	for (UStaticMeshComponent* Mesh : { Video1.Get(), Video2.Get() })
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
	}
}

void ABackVideoActor::BeginPlay()
{
	Super::BeginPlay();
	SetupSlot(Video1, Slot1);
	SetupSlot(Video2, Slot2);
}

void ABackVideoActor::SetupSlot(UStaticMeshComponent* Mesh, const FBackVideoSlot& InSlot)
{
	if (!Mesh || !BaseMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("BackVideo: Mesh or BaseMaterial is None"));
		return;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (InSlot.Texture)
	{
		MID->SetTextureParameterValue(TEXT("Video"), InSlot.Texture);
	}
	Mesh->SetMaterial(0, MID);

	if (InSlot.Player && InSlot.Source)
	{
		if (InSlot.Player->OpenSource(InSlot.Source))
		{
			InSlot.Player->SetLooping(true);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BackVideo: OpenSource failed"));
		}
	}
}