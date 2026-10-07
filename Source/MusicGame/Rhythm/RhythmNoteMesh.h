// デザイナーのモデルができたら、NoteActor の Mesh に Static Mesh を設定すればそちらが使われる。

#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;

namespace RhythmNoteMesh
{
	/*
	   横長の六角形の輪郭（左右の端がとがった形）を返す。XY 平面上、中心が原点
	   幅（Y）= Width、奥行き（X）= Depth、とがった部分の長さ = PointLength
	 */
	void GetHexOutline(float Width, float Depth, float PointLength, TArray<FVector>& OutPoints);

	/*
	   六角形のプレートを Target に作る。底面が Z=0、上面が Z=Thickness
	   UV は上面に、横方向 U（0〜1）・奥行き方向 V（0〜1）で貼られる
	 */
	void BuildHexPlate(UProceduralMeshComponent* Target, float Width, float Depth, float PointLength, float Thickness);
}
