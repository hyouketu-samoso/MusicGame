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
	if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0))
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

	// ミスは見逃したノーツなので、そのまま飛んでいって DestroyProgress で消える
	if (Judgement != ERhythmJudgement::Miss)
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
