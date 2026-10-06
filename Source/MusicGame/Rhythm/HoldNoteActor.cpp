#include "HoldNoteActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 帯を何本の円柱で作るか（多いほど曲線がなめらか）
	constexpr int32 BodySegmentNum = 16;
}

AHoldNoteActor::AHoldNoteActor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// 始点（Mesh）とは別に動かすので、位置・回転・大きさはワールド基準にする
	auto SetupPart = [&](UStaticMeshComponent* Part, UStaticMesh* StaticMesh)
	{
		Part->SetupAttachment(RootComponent);
		Part->SetUsingAbsoluteLocation(true);
		Part->SetUsingAbsoluteRotation(true);
		Part->SetUsingAbsoluteScale(true);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetStaticMesh(StaticMesh);
		Part->SetMaterial(0, BasicMaterial.Object);
	};

	TailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailMesh"));
	SetupPart(TailMesh, SphereMesh.Object);
	TailMesh->SetRelativeScale3D(FVector(0.5f)); // 絶対スケールなので、これがそのままワールドでの大きさ

	for (int32 i = 0; i < BodySegmentNum; ++i)
	{
		UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("BodySegment%d"), i));
		SetupPart(Segment, CylinderMesh.Object);
		BodySegments.Add(Segment);
	}
}

void AHoldNoteActor::ApplyColor(const FLinearColor& Color)
{
	BaseColor = Color;

	Super::ApplyColor(Color);

	if (UMaterialInstanceDynamic* TailMID = TailMesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		TailMID->SetVectorParameterValue(TEXT("Color"), Color);
	}

	// 帯は全部同じ色なので、マテリアルを1つ作って使い回す
	if (!BodyMID && BodySegments.Num() > 0)
	{
		BodyMID = UMaterialInstanceDynamic::Create(BodySegments[0]->GetMaterial(0), this);
		for (UStaticMeshComponent* Segment : BodySegments)
		{
			Segment->SetMaterial(0, BodyMID);
		}
	}
	if (BodyMID)
	{
		// 帯は少し暗くして、始点・終点と見分けやすくする
		BodyMID->SetVectorParameterValue(TEXT("Color"), Color * 0.5f);
	}
}

void AHoldNoteActor::UpdateNote(float T)
{
	// 押している間は始点を判定ラインに留める
	const float HeadT = bHolding ? 1.0f : T;

	// 終点は始点より HoldDuration 分だけ遅れて飛んでくる。出現前は出現地点で待つ
	const float TailT = FMath::Clamp(T - HoldDuration / TravelTime, 0.0f, HeadT);

	SetActorLocation(CalcPosition(HeadT));
	TailMesh->SetWorldLocation(CalcPosition(TailT));

	// 終点 → 始点 の曲線を区切り、各区間に円柱を伸ばして置く
	FVector Prev = CalcPosition(TailT);
	for (int32 i = 0; i < BodySegments.Num(); ++i)
	{
		const float Alpha = static_cast<float>(i + 1) / BodySegments.Num();
		const FVector Next = CalcPosition(FMath::Lerp(TailT, HeadT, Alpha));
		const FVector Delta = Next - Prev;
		const float Length = Delta.Size();

		UStaticMeshComponent* Segment = BodySegments[i];
		Segment->SetVisibility(Length > 1.0f);
		if (Length > 1.0f)
		{
			// 円柱は高さ 100cm・Z 方向が軸なので、区間の向きに Z を合わせて長さぶん伸ばす
			Segment->SetWorldLocationAndRotation((Prev + Next) * 0.5f, FRotationMatrix::MakeFromZ(Delta).Rotator());
			Segment->SetWorldScale3D(FVector(BodyThickness, BodyThickness, Length / 100.0f));
		}

		Prev = Next;
	}

	// 終点まで通り過ぎたら消す（判定待ちの間は消さない）
	if (TailT >= DestroyProgress && !bWaitForJudge)
	{
		Destroy();
	}
}

void AHoldNoteActor::OnJudged(ERhythmJudgement Judgement, ENoteJudgePoint Point)
{
	if (Point == ENoteJudgePoint::HoldStart)
	{
		// 始点を押せたら押し続けの状態へ。終点の判定が出るまで消えない
		bHolding = (Judgement != ERhythmJudgement::Miss);
		return;
	}

	// 終点の判定
	bWaitForJudge = false;
	bHolding = false;

	if (Judgement != ERhythmJudgement::Miss)
	{
		Destroy();
	}
	else
	{
		// 失敗したホールドは暗くして、そのまま飛んでいかせる
		ApplyColor(BaseColor * 0.25f);
	}
}
