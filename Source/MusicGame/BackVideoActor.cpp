// Fill out your copyright notice in the Description page of Project Settings.


#include "BackVideoActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
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

void ABackVideoActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 動画プレイヤーはレベルを開き直しても残るので、このアクターへの登録を外しておく
	GetWorldTimerManager().ClearTimer(SeekFallbackTimer);
	if (Slot1.Player) { Slot1.Player->OnSeekCompleted.RemoveDynamic(this, &ABackVideoActor::HandleSeekCompleted); }
	if (Slot2.Player) { Slot2.Player->OnSeekCompleted.RemoveDynamic(this, &ABackVideoActor::HandleSeekCompleted); }

	Super::EndPlay(EndPlayReason);
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
		// リトライなどでレベルを開き直したとき、同じ動画がすでに開いていれば、開き直さない
		const bool bAlreadyOpen =
			InSlot.Player->IsReady() &&
			InSlot.Player->GetUrl() == InSlot.Source->GetUrl();

		if (bAlreadyOpen)
		{
			const int32 Idx = (&InSlot == &Slot2) ? 1 : 0;

			InSlot.Player->SetLooping(true);

			// 再生中のまま Seek すると取りこぼされることがあるので、いったん止めてから移動する
			InSlot.Player->Pause();
			InSlot.Player->OnSeekCompleted.AddUniqueDynamic(this, &ABackVideoActor::HandleSeekCompleted);

			if (InSlot.Player->Seek(FTimespan::FromSeconds(RestartSeconds)))
			{
				// 移動が終わったら再生する(HandleSeekCompleted)
				bWaitSeek[Idx] = true;

				// 完了の通知が来なくても、1秒後には必ず再生する(止まったままにならないための保険)
				GetWorldTimerManager().SetTimer(SeekFallbackTimer, this, &ABackVideoActor::PlayWaitingVideos, 1.0f, false);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("BackVideo: Seek failed (slot %d)"), Idx);
				InSlot.Player->Play();
			}
		}
		else if (InSlot.Player->OpenSource(InSlot.Source))
		{
			InSlot.Player->SetLooping(true);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BackVideo: OpenSource failed"));
		}
	}
}

void ABackVideoActor::HandleSeekCompleted()
{
	PlayWaitingVideos();
}

void ABackVideoActor::PlayWaitingVideos()
{
	const FBackVideoSlot* Slots[2] = { &Slot1, &Slot2 };
	for (int32 i = 0; i < 2; ++i)
	{
		if (bWaitSeek[i] && Slots[i]->Player)
		{
			bWaitSeek[i] = false;
			Slots[i]->Player->Play();
		}
	}
}

void ABackVideoActor::PauseVideos()
{
	if (Slot1.Player && Slot1.Player->IsPlaying()) { Slot1.Player->Pause(); }
	if (Slot2.Player && Slot2.Player->IsPlaying()) { Slot2.Player->Pause(); }
}

void ABackVideoActor::ResumeVideos()
{
	if (Slot1.Player && Slot1.Player->IsPaused()) { Slot1.Player->Play(); }
	if (Slot2.Player && Slot2.Player->IsPaused()) { Slot2.Player->Play(); }
}