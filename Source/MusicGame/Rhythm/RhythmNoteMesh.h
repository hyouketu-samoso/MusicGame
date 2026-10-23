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

	/*
	   デザイナーのノーツ画像と同じ、左右の辺が斜めの六角形の輪郭を返す（判定枠の表示用）。XY 平面上、中心が原点
	   幅（Y）= Width、奥行き（X）= Depth。bMirror = true で左右反転（右レーン用）
	 */
	void GetSlantedHexOutline(float Width, float Depth, bool bMirror, TArray<FVector>& OutPoints);

	/*
	   画像を貼る平らな四角形を Target に作る（高さ Z = Height、表裏両面）
	   幅（Y）= Width、奥行き（X）= Depth。画像の上が奥（X+）、画像の左が Y- になる UV
	 */
	void BuildTexturedQuad(UProceduralMeshComponent* Target, float Width, float Depth, float Height);

	/*
	   輪郭に沿った側面（厚み）だけを Target の Section 番目に作る（底 Z = Bottom、上 Z = Top、外向き）
	 */
	void BuildPrismSides(UProceduralMeshComponent* Target, int32 Section, const TArray<FVector>& Outline, float Bottom, float Top);
}
