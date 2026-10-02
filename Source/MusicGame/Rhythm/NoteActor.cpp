#include "NoteActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ANoteActor::ANoteActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// エンジン付属の球メッシュを仮置き
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetRelativeScale3D(FVector(0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, BasicMaterial.Object);
	}
}

void ANoteActor::Launch(const FVector& InStartPos, const FVector& InTargetPos, float InArcHeight, float InTravelTime, const FLinearColor& InColor)
{
	StartPos = InStartPos;
	TargetPos = InTargetPos;
	ArcHeight = InArcHeight;
	TravelTime = FMath::Max(InTravelTime, KINDA_SMALL_NUMBER);
	SpawnTime = GetWorld()->GetTimeSeconds();
	bLaunched = true;

	if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), InColor);
	}

	SetActorLocation(CalcPosition(0.0f));
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

	SetActorLocation(CalcPosition(T));

	if (T >= DestroyProgress)
	{
		Destroy();
	}
}

FVector ANoteActor::CalcPosition(float T) const
{
	// 出現地点 → 判定地点へ直線移動（T > 1 でもそのまま手前へ進み続ける）
	FVector Pos = FMath::Lerp(StartPos, TargetPos, T);

	// 放物線で上に持ち上げる。T=0 と T=1 で 0、T=0.5 で ArcHeight になる
	Pos.Z += ArcHeight * 4.0f * T * (1.0f - T);

	return Pos;
}
