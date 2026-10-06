#include "SoundNoteActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

ASoundNoteActor::ASoundNoteActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("Mesh")
	);

	RootComponent = Mesh;

	Mesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	Mesh->SetCastShadow(false);

	Mesh->SetRelativeScale3D(
		FVector(0.5f)
	);

	// 球メッシュ
	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		SphereMesh(
			TEXT("/Engine/BasicShapes/Sphere.Sphere")
		);

	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(
			SphereMesh.Object
		);
	}

	// 基本マテリアル
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>
		BasicMaterial(
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
		);

	if (BasicMaterial.Succeeded())
	{
		Mesh->SetMaterial(
			0,
			BasicMaterial.Object
		);
	}
}

void ASoundNoteActor::Launch(
	const FVector& InStartPos,
	const FVector& InTargetPos,
	float InArcHeight,
	float InTravelTime,
	const FLinearColor& InColor
)
{
	StartPos = InStartPos;
	TargetPos = InTargetPos;

	ArcHeight = InArcHeight;

	TravelTime = FMath::Max(
		InTravelTime,
		KINDA_SMALL_NUMBER
	);

	if (!GetWorld())
	{
		return;
	}

	SpawnTime =
		GetWorld()->GetTimeSeconds();

	bLaunched = true;

	// 色を設定
	if (UMaterialInstanceDynamic* MID =
		Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(
			TEXT("Color"),
			InColor
		);
	}

	SetActorLocation(
		CalcPosition(0.0f)
	);
}

void ASoundNoteActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bLaunched)
	{
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	const double CurrentWorldTime =
		GetWorld()->GetTimeSeconds();

	const float T =
		static_cast<float>(
			(CurrentWorldTime - SpawnTime)
			/ TravelTime
			);

	SetActorLocation(
		CalcPosition(T)
	);

	if (T >= DestroyProgress)
	{
		Destroy();
	}
}

FVector ASoundNoteActor::CalcPosition(float T) const
{
	// 出現地点 → 判定地点
	FVector Position =
		FMath::Lerp(
			StartPos,
			TargetPos,
			T
		);

	// 山なりの軌道
	//
	// T = 0.0 → 0
	// T = 0.5 → ArcHeight
	// T = 1.0 → 0
	//
	Position.Z +=
		ArcHeight *
		4.0f *
		T *
		(1.0f - T);

	return Position;
}