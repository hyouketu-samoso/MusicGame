#include "NoteSpawner.h"
#include "NoteActor.h"
#include "HoldNoteActor.h"
#include "RhythmJudge.h"
#include "Components/ArrowComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

ANoteSpawner::ANoteSpawner()
{
	// 判定ラインの描画のためにTickを使う
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(RootComponent);
		Arrow->ArrowSize = 3.0f;
	}
#endif

	// 判定ラインを映すカメラ（エディタでも位置と視野が表示される）
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	UpdateCameraTransform();

	NoteClass = ANoteActor::StaticClass();
	HoldNoteClass = AHoldNoteActor::StaticClass();

	// 4レーン（左から 左外側 / 左内側 / 右内側 / 右外側）。色はプレイ画面イメージの 水色・青・ピンク・黄
	LaneColors = {
		FLinearColor(0.0f, 0.9f, 1.0f),
		FLinearColor(0.1f, 0.3f, 1.0f),
		FLinearColor(1.0f, 0.2f, 0.8f),
		FLinearColor(1.0f, 0.85f, 0.1f),
	};

	// プレイ画面UI（WBP_GameMain）の判定バー4本に重なる位置。内側のレーンほど手前の V 字。
	// カメラが初期値（距離 700・高さ 400・角度 −15°・視野角 90°）のときに合わせた値なので、
	// カメラを変えたらこちらも合わせ直す
	LaneTargetOffsets = {
		FVector( -60.0f, -300.0f, 0.0f),
		FVector(-205.0f,  -90.0f, 0.0f),
		FVector(-205.0f,   90.0f, 0.0f),
		FVector( -60.0f,  300.0f, 0.0f),
	};
}

void ANoteSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 判定役が指定されていなければレベル内から探し、それでも無ければ作る
	if (!Judge)
	{
		Judge = Cast<ARhythmJudge>(UGameplayStatics::GetActorOfClass(this, ARhythmJudge::StaticClass()));
	}
	if (!Judge)
	{
		Judge = GetWorld()->SpawnActor<ARhythmJudge>(GetActorLocation(), GetActorRotation());
	}

	if (bAutoSpawn)
	{
		GetWorldTimerManager().SetTimer(AutoSpawnTimer, this, &ANoteSpawner::AutoSpawn, SpawnInterval, true, 0.5f);
	}

	if (bAutoCamera)
	{
		// プレイヤーのポーン所持などで視点が後から上書きされないよう、1フレーム待ってから切り替える
		GetWorldTimerManager().SetTimerForNextTick(this, &ANoteSpawner::ActivateAutoCamera);
	}
}

void ANoteSpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdateCameraTransform();
}

void ANoteSpawner::UpdateCameraTransform()
{
	// 判定ラインの中心（このActorの位置）から、後ろへ CameraDistance・上へ CameraHeight の位置
	Camera->SetRelativeLocationAndRotation(FVector(-CameraDistance, 0.0f, CameraHeight), FRotator(CameraPitch, 0.0f, 0.0f));
	Camera->SetFieldOfView(CameraFieldOfView);
}

void ANoteSpawner::ActivateAutoCamera()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	// ポーンの所持やカメラの自動切り替えで視点が戻されないようにする
	PC->bAutoManageActiveCameraTarget = false;

	// ビューターゲットにすると、このActorの Camera コンポーネントの視点になる
	PC->SetViewTarget(this);
}

void ANoteSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowJudgeLine)
	{
		DrawJudgeLine();
	}
}

void ANoteSpawner::AutoSpawn()
{
	// ゲームオーバーになったら発射をやめる
	if (Judge && Judge->IsGameOver())
	{
		GetWorldTimerManager().ClearTimer(AutoSpawnTimer);
		return;
	}

	switch (SpawnPattern)
	{
	case ENoteSpawnPattern::AllLanes:
		SpawnWave();
		break;

	case ENoteSpawnPattern::RandomLane:
		SpawnRandomNote();
		break;
	}
}

void ANoteSpawner::SpawnRandomNote()
{
	// 前のノーツ（ホールド中のレーンなど）と重ならないレーンから選ぶ
	TArray<int32> FreeLanes;
	for (int32 Lane = 0; Lane < LaneColors.Num(); ++Lane)
	{
		if (IsLaneFree(Lane))
		{
			FreeLanes.Add(Lane);
		}
	}
	if (FreeLanes.Num() == 0)
	{
		return;
	}

	const int32 Lane = FreeLanes[FMath::RandRange(0, FreeLanes.Num() - 1)];

	if (FMath::FRand() < HoldChance)
	{
		SpawnHoldNote(Lane, FMath::FRandRange(HoldDurationMin, FMath::Max(HoldDurationMin, HoldDurationMax)));
	}
	else
	{
		SpawnNote(Lane);
	}
}

void ANoteSpawner::SpawnWave()
{
	for (int32 Lane = 0; Lane < LaneColors.Num(); ++Lane)
	{
		if (IsLaneFree(Lane))
		{
			SpawnNote(Lane);
		}
	}
}

void ANoteSpawner::SpawnNote(int32 LaneIndex)
{
	SpawnNoteInternal(NoteClass, LaneIndex, 0.0f);
}

void ANoteSpawner::SpawnHoldNote(int32 LaneIndex, float HoldDuration)
{
	SpawnNoteInternal(HoldNoteClass, LaneIndex, HoldDuration);
}

bool ANoteSpawner::IsLaneFree(int32 LaneIndex) const
{
	if (!LaneBusyUntil.IsValidIndex(LaneIndex))
	{
		return true;
	}

	// 今出すノーツが届く時刻が、前のノーツが届く時刻 + MinLaneGap より後なら空いている
	const double NewHitTime = GetWorld()->GetTimeSeconds() + TravelTime;
	return NewHitTime >= LaneBusyUntil[LaneIndex] + MinLaneGap;
}

void ANoteSpawner::SpawnNoteInternal(TSubclassOf<ANoteActor> Class, int32 LaneIndex, float HoldDuration)
{
	if (!Class || !LaneColors.IsValidIndex(LaneIndex))
	{
		return;
	}

	// 全レーン共通の出現地点（奥の中央）
	const FVector StartPos = GetActorLocation()
		+ GetActorForwardVector() * SpawnDistance
		+ FVector::UpVector * SpawnHeight;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// ノーツの向きはスポナーに合わせる（プレートの横幅がレーンの並びの方向になる）
	ANoteActor* Note = GetWorld()->SpawnActor<ANoteActor>(Class, StartPos, GetActorRotation(), Params);
	if (!Note)
	{
		return;
	}

	// ホールドの長さは Launch より前に決めておく
	AHoldNoteActor* Hold = Cast<AHoldNoteActor>(Note);
	if (Hold)
	{
		Hold->SetHoldDuration(HoldDuration);
	}

	Note->Launch(StartPos, GetLaneTarget(LaneIndex), ArcHeight, TravelTime, LaneColors[LaneIndex], LaneIndex);

	if (Judge)
	{
		Judge->RegisterNote(Note);
	}

	// このレーンが埋まっている時刻を記録する
	if (LaneBusyUntil.Num() < LaneColors.Num())
	{
		LaneBusyUntil.SetNumZeroed(LaneColors.Num());
	}
	LaneBusyUntil[LaneIndex] = Hold ? Hold->GetEndTime() : Note->GetHitTime();
}

FVector ANoteSpawner::GetLaneTarget(int32 LaneIndex) const
{
	// レーンごとの判定位置が設定されていればそれを使う（スポナーの向きに合わせて回転）
	if (LaneTargetOffsets.Num() == LaneColors.Num() && LaneTargetOffsets.IsValidIndex(LaneIndex))
	{
		return GetActorLocation() + GetActorQuat().RotateVector(LaneTargetOffsets[LaneIndex]);
	}

	// 無ければレーンを中央揃えで左右に並べる（4レーンなら -1.5, -0.5, 0.5, 1.5 × LaneSpacing。レーン 0 が左端）
	const float Offset = (LaneIndex - (LaneColors.Num() - 1) * 0.5f) * LaneSpacing;
	return GetActorLocation() + GetActorRightVector() * Offset;
}

void ANoteSpawner::DrawJudgeLine() const
{
	const int32 LaneNum = LaneColors.Num();
	if (LaneNum == 0)
	{
		return;
	}

	const UWorld* World = GetWorld();

	// となりのレーンの判定位置どうしを線でつなぐ（横一列でも V 字でも同じ）
	for (int32 Lane = 0; Lane + 1 < LaneNum; ++Lane)
	{
		DrawDebugLine(World, GetLaneTarget(Lane), GetLaneTarget(Lane + 1), FColor::White, false, -1.0f, 0, 2.0f);
	}

	// 飛ばすノーツの形（クラスの初期値）から輪郭を取り、各レーンの判定位置に描く。
	// ノーツがこの枠にぴったり重なった瞬間がパーフェクト
	const ANoteActor* NoteDefaults = NoteClass ? NoteClass->GetDefaultObject<ANoteActor>() : GetDefault<ANoteActor>();
	TArray<FVector> Outline;
	NoteDefaults->GetPlateOutline(Outline);

	const FQuat Rotation = GetActorQuat();
	for (int32 Lane = 0; Lane < LaneNum; ++Lane)
	{
		const FVector Center = GetLaneTarget(Lane);
		const FColor Color = LaneColors[Lane].ToFColor(true);
		for (int32 i = 0; i < Outline.Num(); ++i)
		{
			const FVector P0 = Center + Rotation.RotateVector(Outline[i]);
			const FVector P1 = Center + Rotation.RotateVector(Outline[(i + 1) % Outline.Num()]);
			DrawDebugLine(World, P0, P1, Color, false, -1.0f, 0, 3.0f);
		}
	}
}
