#include "NoteActor.h"
#include "RhythmNoteMesh.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ANoteActor::ANoteActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	HeadPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivot"));
	HeadPivot->SetupAttachment(RootComponent);

	// 仮の形（BeginPlay で六角形プレートを作る）
	Plate = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Plate"));
	Plate->SetupAttachment(HeadPivot);
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plate->SetCastShadow(false);

	// デザイナーのモデル用（初期状態はモデル無し）
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(HeadPivot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	// "Color" パラメータを持つエンジン付属マテリアルを仮置き
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		NoteMaterial = BasicMaterial.Object;
	}
}

void ANoteActor::BeginPlay()
{
	Super::BeginPlay();

	SetupVisual(HeadPivot, Plate, Mesh);
}

void ANoteActor::SetupVisual(USceneComponent* Pivot, UProceduralMeshComponent* InPlate, UStaticMeshComponent* InMesh) const
{
	// プラスに傾けると、上面がカメラ側（Actor の後ろ）を向く
	Pivot->SetRelativeRotation(FRotator(PlateTilt, 0.0f, 0.0f));

	if (InMesh->GetStaticMesh())
	{
		// モデルが設定されていればそちらを使う
		InPlate->SetVisibility(false);
	}
	else
	{
		RhythmNoteMesh::BuildHexPlate(InPlate, PlateWidth, PlateDepth, PlatePointLength, PlateThickness);
		InPlate->SetMaterial(0, NoteMaterial);
		InMesh->SetVisibility(false);
	}
}

void ANoteActor::GetPlateOutline(TArray<FVector>& OutPoints) const
{
	RhythmNoteMesh::GetHexOutline(PlateWidth, PlateDepth, PlatePointLength, OutPoints);

	const FRotator Tilt(PlateTilt, 0.0f, 0.0f);
	for (FVector& Point : OutPoints)
	{
		Point = Tilt.RotateVector(Point);
	}
}

void ANoteActor::Launch(const FVector& InStartPos, const FVector& InTargetPos, float InArcHeight, float InTravelTime, const FLinearColor& InColor, int32 InLaneIndex)
{
	LaneIndex = InLaneIndex;
	StartPos = InStartPos;
	TargetPos = InTargetPos;
	ArcHeight = InArcHeight;
	TravelTime = FMath::Max(InTravelTime, KINDA_SMALL_NUMBER);
	SpawnTime = GetWorld()->GetTimeSeconds();
	bLaunched = true;

	ApplyColor(InColor);

	UpdateNote(0.0f);
}

void ANoteActor::ApplyColor(const FLinearColor& Color)
{
	SetMeshColor(Plate, Color);
	SetMeshColor(Mesh, Color);
}

void ANoteActor::SetMeshColor(UMeshComponent* Target, const FLinearColor& Color)
{
	if (!Target || !Target->IsVisible())
	{
		return;
	}

	if (UMaterialInstanceDynamic* MID = Target->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void ANoteActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bLaunched)
	{
		return;
	}

	// ＝＝メモ＝＝
	//「出現してからの時間」で位置を決定。
	// 曲の再生位置に置き換えるだけで音とズレなくなる。
	const float T = static_cast<float>((GetWorld()->GetTimeSeconds() - SpawnTime) / TravelTime);

	UpdateNote(T);
}

void ANoteActor::UpdateNote(float T)
{
	SetActorLocation(CalcPosition(T));

	// 判定待ちの間は消さない（ミス判定が出る前に消えないように）
	if (T >= DestroyProgress && !bWaitForJudge)
	{
		Destroy();
	}
}

void ANoteActor::OnJudged(ERhythmJudgement Judgement, ENoteJudgePoint Point)
{
	bWaitForJudge = false;

	// 判定が確定したら削除する（Perfect〜Bad はヒット、Miss も「Miss処理後に削除」が仕様）
	Destroy();
}

FVector ANoteActor::CalcPosition(float T) const
{
	// 出現地点 → 判定地点へ直線移動（T > 1 でもそのまま手前へ進み続ける）
	FVector Pos = FMath::Lerp(StartPos, TargetPos, T);

	// 放物線で上に持ち上げる。T=0 と T=1 で 0、T=0.5 で ArcHeight になる
	Pos.Z += ArcHeight * 4.0f * T * (1.0f - T);

	return Pos;
}
