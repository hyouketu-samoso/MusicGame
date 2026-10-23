#include "NoteActor.h"
#include "RhythmNoteMesh.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
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

	// 画像のノーツ用：透明部分を抜いて、画像をそのまま表示するマテリアル（Paper2D 付属）
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SpriteMaterial(TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	if (SpriteMaterial.Succeeded())
	{
		NoteTextureMaterial = SpriteMaterial.Object;
	}
}

void ANoteActor::SetupTextureVisual()
{
	if (Mesh->GetStaticMesh())
	{
		return;   // モデルが設定されていればモデル優先
	}

	BuildTexturePlate(Plate);
	Plate->SetVisibility(true);
}

void ANoteActor::BuildTexturePlate(UProceduralMeshComponent* Target) const
{
	if (!Target || !NoteTexture)
	{
		return;
	}

	// 画像の中の六角形の幅が PlateWidth になるように、画像の縦横比のまま板を大きくする
	const float QuadWidth = PlateWidth / FMath::Max(TextureHexWidthRatio, 0.01f);
	const float Aspect = static_cast<float>(NoteTexture->GetSurfaceHeight()) / FMath::Max(static_cast<float>(NoteTexture->GetSurfaceWidth()), 1.0f);
	const float QuadDepth = QuadWidth * Aspect;

	// 画像は厚みの上面に貼る（section 0）
	RhythmNoteMesh::BuildTexturedQuad(Target, QuadWidth, QuadDepth, TextureThickness + 0.1f);

	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(NoteTextureMaterial, Target))
	{
		MID->SetTextureParameterValue(NoteTextureParameterName, NoteTexture);
		Target->SetMaterial(0, MID);
	}

	// 厚み：画像の六角形の形に沿った側面（section 1）。色はレーン色（ApplyColor で塗る）
	if (TextureThickness > 0.0f)
	{
		TArray<FVector> Outline;
		RhythmNoteMesh::GetSlantedHexOutline(PlateWidth, QuadDepth * TextureHexHeightRatio, bTextureRightSide, Outline);
		RhythmNoteMesh::BuildPrismSides(Target, 1, Outline, 0.0f, TextureThickness);
		Target->SetMaterial(1, NoteMaterial);
	}
}

float ANoteActor::CalcTilt(float T) const
{
	// 出現時は StartTilt で起こしておき、判定地点に着くまでにゆっくり PlateTilt まで寝かせる
	const float Alpha = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(T, 0.0f, 1.0f));
	return FMath::Lerp(StartTilt, PlateTilt, Alpha);
}

void ANoteActor::GetTexturePlateOutline(const UTexture2D* InTexture, bool bRightSide, TArray<FVector>& OutPoints) const
{
	if (!InTexture)
	{
		GetPlateOutline(OutPoints);
		return;
	}

	const float QuadWidth = PlateWidth / FMath::Max(TextureHexWidthRatio, 0.01f);
	const float Aspect = static_cast<float>(InTexture->GetSurfaceHeight()) / FMath::Max(static_cast<float>(InTexture->GetSurfaceWidth()), 1.0f);
	const float HexDepth = QuadWidth * Aspect * TextureHexHeightRatio;

	// 左レーン用の画像の形が基準。右レーン用は左右反転
	RhythmNoteMesh::GetSlantedHexOutline(PlateWidth, HexDepth, bRightSide, OutPoints);

	const FRotator Tilt(PlateTilt, 0.0f, 0.0f);
	for (FVector& Point : OutPoints)
	{
		Point = Tilt.RotateVector(Point + FVector(0.0f, 0.0f, TextureThickness));
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
	StartRotation = GetActorQuat();
	SetActorScale3D(FVector(NoteScale));

	// 画像が設定されていれば、六角形の板を画像の板に作り直す（色を塗る前に）
	if (NoteTexture)
	{
		SetupTextureVisual();
	}
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

	// 画像のノーツは section 1（厚みの側面）にも色を塗る。"Color" が無いマテリアル（画像用）では何も起きない
	for (int32 Index = 0; Index < Target->GetNumMaterials(); ++Index)
	{
		if (UMaterialInstanceDynamic* MID = Target->CreateAndSetMaterialInstanceDynamic(Index))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
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
	SetActorLocationAndRotation(CalcPosition(T), CalcRotation(T));
	HeadPivot->SetRelativeRotation(FRotator(CalcTilt(T), 0.0f, 0.0f));

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

FQuat ANoteActor::CalcRotation(float T) const
{
	// 出現時は横向き、判定地点で ArrivalYaw。途中はゆっくり回り始めてゆっくり合わせる
	const float Alpha = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(T, 0.0f, 1.0f));
	return StartRotation * FRotator(0.0f, ArrivalYaw * Alpha, 0.0f).Quaternion();
}
