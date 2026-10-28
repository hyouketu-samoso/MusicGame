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
#include "input/RhythmPlayerController.h"

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
			// タイトルに戻ったとき(リトライ以外)は、流さずに 0 秒で止めておく
			if (!IsRetryStart())
			{
				ParkAtTitle(InSlot);
				return;
			}

			const int32 Idx = (&InSlot == &Slot2) ? 1 : 0;

			InSlot.Player->SetLooping(bLoopVideo);

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
			InSlot.Player->SetLooping(bLoopVideo);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BackVideo: OpenSource failed"));
		}
	}
}

void ABackVideoActor::HandleSeekCompleted()
{
	// タイトルで開始位置へ移動していた動画は、移動が終わった
	const FBackVideoSlot* Slots[2] = { &Slot1, &Slot2 };
	for (int32 i = 0; i < 2; ++i)
	{
		// 移動先に近い位置になったら完了とみなす(動画によっては、少しずれた位置に止まるため、余裕を持たせる)
		if (bTitleSeekPending[i] && Slots[i]->Player &&
			FMath::Abs(Slots[i]->Player->GetTime().GetTotalSeconds() - TitleStartSeconds) < 0.5)
		{
			bTitleSeekPending[i] = false;
		}
	}

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

bool ABackVideoActor::IsRetryStart() const
{
	// リトライかどうかは、RhythmPlayerController が持っている(印は読むと消えるので、コントローラーが覚えている)
	if (const UWorld* World = GetWorld())
	{
		if (ARhythmPlayerController* PC = Cast<ARhythmPlayerController>(World->GetFirstPlayerController()))
		{
			return PC->IsRetryStart();
		}
	}

	// 分からないときは、これまでどおりリトライの動き(1.5秒から再生)にする
	UE_LOG(LogTemp, Warning, TEXT("BackVideo: PlayerController not found, treated as retry"));
	return true;
}

// タイトル用: 止めて TitleStartSeconds へ移動だけしておく(再生はしない)
void ABackVideoActor::ParkAtTitle(const FBackVideoSlot& InSlot)
{
	UMediaPlayer* P = InSlot.Player;
	if (!P) { return; }

	const int32 Idx = (&InSlot == &Slot2) ? 1 : 0;

	P->SetLooping(bLoopVideo);
	P->Pause();

	// すでにその位置なら、移動しない(同じ位置への移動は完了の通知が来ないことがあるため)
	if (FMath::Abs(P->GetTime().GetTotalSeconds() - TitleStartSeconds) < 0.05)
	{
		UE_LOG(LogTemp, Warning, TEXT("BackVideo: ParkAtTitle slot %d (already at start)"), Idx);
		return;
	}

	P->OnSeekCompleted.AddUniqueDynamic(this, &ABackVideoActor::HandleSeekCompleted);

	if (P->Seek(FTimespan::FromSeconds(TitleStartSeconds)))
	{
		bTitleSeekPending[Idx] = true;
		UE_LOG(LogTemp, Warning, TEXT("BackVideo: ParkAtTitle slot %d seeking to %.2f"), Idx, TitleStartSeconds);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BackVideo: ParkAtTitle seek failed (slot %d)"), Idx);
	}
}

// タイトルからスタート: 止めておいた動画を再生する(開始位置への移動はタイトルで済んでいる)
void ABackVideoActor::ReleaseFromTitle()
{
	const FBackVideoSlot* Slots[2] = { &Slot1, &Slot2 };
	bool bNeedFallback = false;

	for (int32 i = 0; i < 2; ++i)
	{
		UMediaPlayer* P = Slots[i]->Player;
		if (!P) { continue; }

		UE_LOG(LogTemp, Warning, TEXT("BackVideo: ReleaseFromTitle slot %d (ready=%d time=%.2f seekPending=%d)"),
			i, P->IsReady(), P->GetTime().GetTotalSeconds(), bTitleSeekPending[i]);

		if (!P->IsReady()) { continue; }

		P->SetLooping(bLoopVideo);

		// 通知が来ていなくても、すでに開始位置の近くまで移動できていれば、移動は終わっているとみなす
		if (bTitleSeekPending[i] &&
			FMath::Abs(P->GetTime().GetTotalSeconds() - TitleStartSeconds) < 0.5)
		{
			bTitleSeekPending[i] = false;
		}

		if (bTitleSeekPending[i])
		{
			// 開始位置への移動がまだ終わっていないとき(スタートを押すのが早かった)は、終わってから再生する
			bWaitSeek[i] = true;
			bNeedFallback = true;
		}
		else
		{
			P->Play();
		}
	}

	if (bNeedFallback)
	{
		GetWorldTimerManager().SetTimer(SeekFallbackTimer, this, &ABackVideoActor::PlayWaitingVideos, 1.0f, false);
	}
}