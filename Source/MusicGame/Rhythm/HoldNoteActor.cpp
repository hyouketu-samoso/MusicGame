#include "HoldNoteActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"

namespace
{
	// 帯を曲線に沿って何区間に分けるか（多いほどなめらか）
	constexpr int32 BodySegmentNum = 16;
}

AHoldNoteActor::AHoldNoteActor()
{
	// 終点は始点とは別の位置に置くので、位置だけワールド基準にする（向きは Actor に合わせる）
	TailPivot = CreateDefaultSubobject<USceneComponent>(TEXT("TailPivot"));
	TailPivot->SetupAttachment(RootComponent);
	TailPivot->SetUsingAbsoluteLocation(true);

	TailPlate = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TailPlate"));
	TailPlate->SetupAttachment(TailPivot);
	TailPlate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TailPlate->SetCastShadow(false);

	TailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailMesh"));
	TailMesh->SetupAttachment(TailPivot);
	TailMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TailMesh->SetCastShadow(false);

	// 帯の頂点はワールド座標で作るので、帯自体は原点・回転なしに固定する
	Body = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetUsingAbsoluteLocation(true);
	Body->SetUsingAbsoluteRotation(true);
	Body->SetUsingAbsoluteScale(true);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);
}

void AHoldNoteActor::BeginPlay()
{
	Super::BeginPlay();

	SetupVisual(TailPivot, TailPlate, TailMesh);

	Body->SetWorldTransform(FTransform::Identity);
	Body->SetMaterial(0, NoteMaterial);
}

void AHoldNoteActor::ApplyColor(const FLinearColor& Color)
{
	BaseColor = Color;

	Super::ApplyColor(Color);

	SetMeshColor(TailPlate, Color);
	SetMeshColor(TailMesh, Color);

	// 帯は少し暗くして、始点・終点と見分けやすくする
	SetMeshColor(Body, Color * 0.5f);
}

void AHoldNoteActor::UpdateNote(float T)
{
	// 押している間は始点を判定ラインに留める
	const float HeadT = bHolding ? 1.0f : T;

	// 終点は始点より HoldDuration 分だけ遅れて飛んでくる。出現前は出現地点で待つ
	const float TailT = FMath::Clamp(T - HoldDuration / TravelTime, 0.0f, HeadT);

	SetActorLocation(CalcPosition(HeadT));
	TailPivot->SetWorldLocation(CalcPosition(TailT));

	UpdateBody(TailT, HeadT);

	// 終点まで通り過ぎたら消す（判定待ちの間は消さない）
	if (TailT >= DestroyProgress && !bWaitForJudge)
	{
		Destroy();
	}
}

void AHoldNoteActor::UpdateBody(float TailT, float HeadT)
{
	// 帯は上面・裏面の両方から見えるように、表と裏で別々に頂点を持つ
	const int32 PointNum = BodySegmentNum + 1;
	const FVector Right = GetActorRightVector() * (PlateWidth * BodyWidthRatio * 0.5f);
	const FVector Lift(0.0f, 0.0f, PlateThickness * 0.5f); // プレートの厚みの真ん中を通す

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	Vertices.Reserve(PointNum * 4);
	Normals.Reserve(PointNum * 4);
	UVs.Reserve(PointNum * 4);

	TArray<FVector> Centers;
	Centers.Reserve(PointNum);
	for (int32 i = 0; i < PointNum; ++i)
	{
		Centers.Add(CalcPosition(FMath::Lerp(TailT, HeadT, static_cast<float>(i) / BodySegmentNum)) + Lift);
	}

	for (int32 Side = 0; Side < 2; ++Side)
	{
		for (int32 i = 0; i < PointNum; ++i)
		{
			// 曲線の進む向きとレーンの横方向から、帯の面の向き（上向き）を求める
			const FVector Along = Centers[FMath::Min(i + 1, PointNum - 1)] - Centers[FMath::Max(i - 1, 0)];
			FVector Up = (Right ^ Along).GetSafeNormal();
			if (Up.IsNearlyZero())
			{
				Up = FVector::UpVector;
			}
			if (Up.Z < 0.0f)
			{
				Up = -Up;
			}

			const FVector Normal = (Side == 0) ? Up : -Up;
			const float V = static_cast<float>(i) / BodySegmentNum;

			Vertices.Add(Centers[i] - Right);
			Vertices.Add(Centers[i] + Right);
			Normals.Add(Normal);
			Normals.Add(Normal);
			UVs.Add(FVector2D(0.0f, V));
			UVs.Add(FVector2D(1.0f, V));
		}
	}

	if (!bBodyCreated)
	{
		// 区間ごとに四角形（三角形2枚）。表は上から、裏は下から見える順番にする
		// （UE では (C-A)×(B-A) が表側を向く。帯は終点 → 始点（手前向き）に並んでいるので、
		//   左 → 次の左 → 右 の順で上向きになる）
		TArray<int32> Triangles;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const int32 Base = Side * PointNum * 2;
			for (int32 i = 0; i < BodySegmentNum; ++i)
			{
				const int32 L0 = Base + i * 2;
				const int32 R0 = L0 + 1;
				const int32 L1 = L0 + 2;
				const int32 R1 = L0 + 3;

				if (Side == 0)
				{
					Triangles.Append({ L0, L1, R0, R0, L1, R1 });
				}
				else
				{
					Triangles.Append({ L0, R0, L1, R0, R1, L1 });
				}
			}
		}

		Body->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, {}, false);
		bBodyCreated = true;
	}
	else
	{
		Body->UpdateMeshSection(0, Vertices, Normals, UVs, {}, {});
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
