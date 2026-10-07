#include "RhythmNoteMesh.h"
#include "ProceduralMeshComponent.h"

namespace
{
	struct FMeshData
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;

		void AddTriangle(FVector A, FVector B, FVector C, FVector2D UVA, FVector2D UVB, FVector2D UVC, const FVector& OutwardDir)
		{
			if ((((C - A) ^ (B - A)) | OutwardDir) < 0.0f)
			{
				Swap(B, C);
				Swap(UVB, UVC);
			}

			const FVector Normal = ((C - A) ^ (B - A)).GetSafeNormal();
			const int32 Base = Vertices.Num();

			Vertices.Append({ A, B, C });
			Normals.Append({ Normal, Normal, Normal });
			UVs.Append({ UVA, UVB, UVC });
			Triangles.Append({ Base, Base + 1, Base + 2 });
		}
	};
}

void RhythmNoteMesh::GetHexOutline(float Width, float Depth, float PointLength, TArray<FVector>& OutPoints)
{
	const float HalfW = Width * 0.5f;
	const float HalfD = Depth * 0.5f;
	const float Point = FMath::Clamp(PointLength, 0.0f, HalfW);

	OutPoints = {
		FVector(0.0f,   -HalfW,         0.0f),
		FVector(HalfD,  -HalfW + Point, 0.0f),
		FVector(HalfD,   HalfW - Point, 0.0f),
		FVector(0.0f,    HalfW,         0.0f),
		FVector(-HalfD,  HalfW - Point, 0.0f),
		FVector(-HalfD, -HalfW + Point, 0.0f),
	};
}

void RhythmNoteMesh::BuildHexPlate(UProceduralMeshComponent* Target, float Width, float Depth, float PointLength, float Thickness)
{
	if (!Target)
	{
		return;
	}

	TArray<FVector> Outline;
	GetHexOutline(Width, Depth, PointLength, Outline);

	const FVector Up(0.0f, 0.0f, Thickness);
	const int32 Num = Outline.Num();

	// 上面の UV：横方向が U、奥行き方向が V
	auto CalcUV = [&](const FVector& P)
	{
		return FVector2D(P.Y / Width + 0.5f, 0.5f - P.X / Depth);
	};

	FMeshData Data;

	// 上面と底面（中心から扇形に三角形を並べる）
	const FVector Center = FVector::ZeroVector;
	for (int32 i = 0; i < Num; ++i)
	{
		const FVector& P0 = Outline[i];
		const FVector& P1 = Outline[(i + 1) % Num];

		Data.AddTriangle(Center + Up, P0 + Up, P1 + Up, CalcUV(Center), CalcUV(P0), CalcUV(P1), FVector::UpVector);
		Data.AddTriangle(Center, P0, P1, CalcUV(Center), CalcUV(P0), CalcUV(P1), FVector::DownVector);
	}

	// 側面（辺ごとに四角形）
	for (int32 i = 0; i < Num; ++i)
	{
		const FVector& P0 = Outline[i];
		const FVector& P1 = Outline[(i + 1) % Num];
		const FVector Outward = ((P0 + P1) * 0.5f).GetSafeNormal2D();

		Data.AddTriangle(P0, P1, P1 + Up, FVector2D(0, 1), FVector2D(1, 1), FVector2D(1, 0), Outward);
		Data.AddTriangle(P0, P1 + Up, P0 + Up, FVector2D(0, 1), FVector2D(1, 0), FVector2D(0, 0), Outward);
	}

	Target->CreateMeshSection(0, Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, {}, {}, false);
}
