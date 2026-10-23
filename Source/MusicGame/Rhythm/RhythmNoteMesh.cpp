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

void RhythmNoteMesh::GetSlantedHexOutline(float Width, float Depth, bool bMirror, TArray<FVector>& OutPoints)
{
	// デザイナーのノーツ画像（左レーン用：WBP_LeftRedNotes）の白い縁の外形を、
	// 六角形の幅・高さを 1 として読み取った値（左上・右上・右・右下・左下・左）。
	// 上辺は左寄り、下辺は右寄りにずれた、左右の辺が斜めの六角形。右レーン用は左右反転
	static const FVector2D Normalized[] =
	{
		FVector2D(-0.399f, -0.5f),
		FVector2D( 0.239f, -0.5f),
		FVector2D( 0.5f,    0.0f),
		FVector2D( 0.399f,  0.5f),
		FVector2D(-0.239f,  0.5f),
		FVector2D(-0.5f,    0.0f),
	};

	OutPoints.Reset();
	for (const FVector2D& N : Normalized)
	{
		// 画像の上 = 奥（X+）、画像の右 = Y+
		const float U = bMirror ? -N.X : N.X;
		OutPoints.Add(FVector(-N.Y * Depth, U * Width, 0.0f));
	}
}

void RhythmNoteMesh::BuildTexturedQuad(UProceduralMeshComponent* Target, float Width, float Depth, float Height)
{
	if (!Target)
	{
		return;
	}

	// 画像の上 = 奥（X+）、画像の左 = Y-。上からも下からも見えるように表と裏を作る
	const float HX = Depth * 0.5f;
	const float HY = Width * 0.5f;
	const FVector P00(HX, -HY, Height);   // 画像の左上
	const FVector P10(HX, HY, Height);    // 右上
	const FVector P11(-HX, HY, Height);   // 右下
	const FVector P01(-HX, -HY, Height);  // 左下

	FMeshData Data;
	Data.AddTriangle(P00, P10, P11, FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector::UpVector);
	Data.AddTriangle(P00, P11, P01, FVector2D(0, 0), FVector2D(1, 1), FVector2D(0, 1), FVector::UpVector);
	Data.AddTriangle(P00, P10, P11, FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector::DownVector);
	Data.AddTriangle(P00, P11, P01, FVector2D(0, 0), FVector2D(1, 1), FVector2D(0, 1), FVector::DownVector);

	// スプライト用マテリアルは頂点カラーを掛けるので白にする
	TArray<FColor> Colors;
	Colors.Init(FColor::White, Data.Vertices.Num());

	Target->ClearAllMeshSections();
	Target->CreateMeshSection(0, Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, Colors, {}, false);
}

void RhythmNoteMesh::BuildPrismSides(UProceduralMeshComponent* Target, int32 Section, const TArray<FVector>& Outline, float Bottom, float Top)
{
	const int32 Num = Outline.Num();
	if (!Target || Num < 3)
	{
		return;
	}

	const FVector Down(0.0f, 0.0f, Bottom);
	const FVector Up(0.0f, 0.0f, Top);

	// 輪郭の中心から外へ向かう向きを、各辺の表側にする
	FVector Center = FVector::ZeroVector;
	for (const FVector& P : Outline)
	{
		Center += P;
	}
	Center /= static_cast<float>(Num);

	FMeshData Data;
	for (int32 i = 0; i < Num; ++i)
	{
		const FVector P0(Outline[i].X, Outline[i].Y, 0.0f);
		const FVector P1(Outline[(i + 1) % Num].X, Outline[(i + 1) % Num].Y, 0.0f);
		const FVector Outward = ((P0 + P1) * 0.5f - FVector(Center.X, Center.Y, 0.0f)).GetSafeNormal2D();

		Data.AddTriangle(P0 + Down, P1 + Down, P1 + Up, FVector2D(0, 1), FVector2D(1, 1), FVector2D(1, 0), Outward);
		Data.AddTriangle(P0 + Down, P1 + Up, P0 + Up, FVector2D(0, 1), FVector2D(1, 0), FVector2D(0, 0), Outward);
	}

	Target->CreateMeshSection(Section, Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, {}, {}, false);
}
