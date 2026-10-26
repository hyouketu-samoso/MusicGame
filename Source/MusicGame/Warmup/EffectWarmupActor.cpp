#include "EffectWarmupActor.h"

#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Containers/Ticker.h"
#include "TimerManager.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Components/SceneComponent.h"

AEffectWarmupActor::AEffectWarmupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 位置を持てるように、ルートを作る
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void AEffectWarmupActor::BeginPlay()
{
	Super::BeginPlay();

	// 少し待ってから始める(曲選択の表示などと、同じフレームで重くならないように)
	// ※ ポーズ中は、タイマーが進まないので、待ち時間は実時間のティッカーで数える
	const float Delay = StartDelay;
	TWeakObjectPtr<AEffectWarmupActor> WeakThis(this);
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis](float) -> bool
			{
				if (WeakThis.IsValid())
				{
					WeakThis->RunWarmup();
				}
				return false;   // 1回だけ
			}),
		Delay);
}

void AEffectWarmupActor::RunWarmup()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// エフェクトを出す位置を決める
	if (bSpawnAtActorLocation)
	{
		// このアクターの位置に出す(レベルで、画面の範囲内の見えない場所に置く)
		SpawnLocation = GetActorLocation();
		SpawnRotation = GetActorRotation();
	}
	else
	{
		// カメラの前の位置に出す(画面の外だと描かれず、準備が走らないため、画面の中の小さな点に出す)
		FVector CamLoc = GetActorLocation();
		FRotator CamRot = FRotator::ZeroRotator;
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->GetPlayerViewPoint(CamLoc, CamRot);
		}
		SpawnLocation = CamLoc + CamRot.Vector() * DistanceFromCamera;
		SpawnRotation = CamRot;
	}

	// 読み込むアセットの一覧
	TArray<FSoftObjectPath> Paths;
	for (const TSoftObjectPtr<UNiagaraSystem>& Soft : WarmupSystems)
	{
		if (!Soft.IsNull())
		{
			Paths.Add(Soft.ToSoftObjectPath());
		}
	}
	if (Paths.Num() == 0)
	{
		return;
	}

	// 裏で読み込む(LoadSynchronous と違い、読み込み中にゲームが固まらない)
	TWeakObjectPtr<AEffectWarmupActor> WeakThis(this);
	LoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateLambda([WeakThis]()
			{
				if (AEffectWarmupActor* Self = WeakThis.Get())
				{
					Self->NextIndex = 0;
					Self->SpawnNext();
				}
			}));
}

// 1つ出して、次を少し待ってから出す(最後まで出したら、一定時間後に、すべて消す)
void AEffectWarmupActor::SpawnNext()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (WarmupSystems.IsValidIndex(NextIndex))
	{
		if (UNiagaraSystem* System = WarmupSystems[NextIndex].Get())
		{
			// プールは使わず、自動で消えないようにして出す(消すのは、あとでこちらが行う)
			UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World, System, SpawnLocation, SpawnRotation, FVector(WarmupScale),
				/*bAutoDestroy=*/ false, /*bAutoActivate=*/ true,
				ENCPoolMethod::None, /*bPreCullCheck=*/ false);

			if (Comp)
			{
				// ポーズ中でも動かして、描画の準備を走らせる
				Comp->PrimaryComponentTick.bTickEvenWhenPaused = true;
				Spawned.Add(Comp);
			}
		}
		++NextIndex;

		// 次のエフェクトを、少し待ってから出す(ポーズ中でも進むよう、実時間で数える)
		TWeakObjectPtr<AEffectWarmupActor> WeakThis(this);
		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([WeakThis](float) -> bool
				{
					if (AEffectWarmupActor* Self = WeakThis.Get())
					{
						Self->SpawnNext();
					}
					return false;
				}),
			0.05f);
		return;
	}

	// 全部出したので、一定時間後に消す
	TWeakObjectPtr<AEffectWarmupActor> WeakThis(this);
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis](float) -> bool
			{
				if (AEffectWarmupActor* Self = WeakThis.Get())
				{
					Self->CleanupSpawned();
				}
				return false;
			}),
		KeepSeconds);
}

void AEffectWarmupActor::CleanupSpawned()
{
	for (TWeakObjectPtr<UNiagaraComponent>& C : Spawned)
	{
		if (C.IsValid())
		{
			C->DeactivateImmediate();
			C->DestroyComponent();
		}
	}
	Spawned.Reset();
}