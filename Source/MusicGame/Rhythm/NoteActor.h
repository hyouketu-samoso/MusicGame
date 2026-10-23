// ノーツ1個分のActor。出現地点から判定地点まで放物線を描いて飛ぶ。
// 見た目は横長の六角形プレート（仮の形をプログラムで作る）。
// Mesh に Static Mesh を設定すると、仮の形の代わりにそのモデルを使う。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RhythmTypes.h"
#include "NoteActor.generated.h"

class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMeshComponent;
class UTexture2D;

UCLASS()
class ANoteActor : public AActor
{
	GENERATED_BODY()

public:

	ANoteActor();

	/*
	   飛行パラメータを設定して飛ばし始める（Spawn直後に呼ぶ）
	   @param InStartPos    出現地点（ワールド座標）
	   @param InTargetPos   判定地点（ワールド座標）
	   @param InArcHeight   山なりの高さ（cm）
	   @param InTravelTime  出現してから判定地点に届くまでの秒数
	   @param InColor       ノーツの色
	   @param InLaneIndex   レーン番号
	 */
	void Launch(const FVector& InStartPos, const FVector& InTargetPos, float InArcHeight, float InTravelTime, const FLinearColor& InColor, int32 InLaneIndex);

	// 判定地点に届いたときの向き（出現時の向きからのヨー回転、度）。Launch より前に呼ぶ。
	// 出現したときは横向きで、判定地点に近づくにつれてこの角度まで回る
	void SetArrivalYaw(float InYaw) { ArrivalYaw = InYaw; }

	// ノーツの大きさ（倍率）。Launch より前に呼ぶ。遠くて小さく見えるレーンを大きくするのに使う
	void SetNoteScale(float InScale) { NoteScale = FMath::Max(InScale, 0.01f); }

	// ノーツの画像（デザイナーのノーツ画像）。Launch より前に呼ぶ。
	// 設定すると、六角形の板の代わりに画像を貼った板で表示する（Mesh にモデルがあるときはモデル優先）
	// bRightSide = 右レーン用（画像の六角形は右レーン用が左右反転の形。厚みの側面をその形に合わせる）
	void SetNoteTexture(UTexture2D* InTexture, bool bRightSide) { NoteTexture = InTexture; bTextureRightSide = bRightSide; }

	// 画像の板で表示したときの、画像の中の六角形の輪郭（Actor 基準、判定枠の表示用）
	// bRightSide = 右レーン用（画像の六角形は右レーン用が左右反転の形）
	void GetTexturePlateOutline(const UTexture2D* InTexture, bool bRightSide, TArray<FVector>& OutPoints) const;

	virtual void Tick(float DeltaTime) override;

	// レーン番号
	int32 GetLaneIndex() const { return LaneIndex; }

	// 判定ラインに届く時刻（ワールド時間）。ホールドなら始点が届く時刻
	double GetHitTime() const { return SpawnTime + TravelTime; }

	// ホールドノーツか
	virtual bool IsHold() const { return false; }

	// true の間は判定が出るまで消えない（判定役に登録されたら true になる）
	void SetWaitForJudge(bool bWait) { bWaitForJudge = bWait; }

	// 判定が確定したときに判定役から呼ばれる。判定が確定したらその場で消える（Miss も含む）
	virtual void OnJudged(ERhythmJudgement Judgement, ENoteJudgePoint Point);

	// 判定対象から外す（ゲームオーバー時など）。そのまま飛んでいって消える
	virtual void Abandon() { bWaitForJudge = false; }

	// プレートの輪郭（Actor 基準の座標、傾きも反映済み）。判定位置の目安表示に使う
	void GetPlateOutline(TArray<FVector>& OutPoints) const;

protected:

	virtual void BeginPlay() override;

	// プレートの傾きをかけるための親（Plate と Mesh はこの下に付く）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Note")
	TObjectPtr<USceneComponent> HeadPivot;

	// 仮の形（横長の六角形プレート）。Mesh にモデルが無いときに使う
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Note")
	TObjectPtr<UProceduralMeshComponent> Plate;

	// デザイナーのモデル用。Static Mesh を設定すると Plate の代わりにこちらを表示する
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Note")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 仮の形に使うマテリアル。"Color" という Vector パラメータがあればレーンの色が入る
	UPROPERTY(EditDefaultsOnly, Category="Note|Shape")
	TObjectPtr<UMaterialInterface> NoteMaterial;

	// プレートの横幅（レーンの並びの方向、cm）
	UPROPERTY(EditDefaultsOnly, Category="Note|Shape", meta=(ClampMin=1.0, Units="cm"))
	float PlateWidth = 120.0f;

	// プレートの奥行き（飛んでくる方向、cm）
	UPROPERTY(EditDefaultsOnly, Category="Note|Shape", meta=(ClampMin=1.0, Units="cm"))
	float PlateDepth = 35.0f;

	// 左右のとがった部分の長さ（cm）
	UPROPERTY(EditDefaultsOnly, Category="Note|Shape", meta=(ClampMin=0.0, Units="cm"))
	float PlatePointLength = 22.0f;

	// プレートの厚み（cm）
	UPROPERTY(EditDefaultsOnly, Category="Note|Shape", meta=(ClampMin=0.1, Units="cm"))
	float PlateThickness = 6.0f;

	// プレートの傾き（度）。0 = 道路に寝かせる、90 = カメラ側へ立てる
	UPROPERTY(EditDefaultsOnly, Category="Note|Shape", meta=(ClampMin=-90.0, ClampMax=90.0, Units="deg"))
	float PlateTilt = 0.0f;

	// 出現したときの傾き（度）。遠くでは板をカメラ側へ起こして見やすくし、判定地点に着くまでに PlateTilt まで寝かせる。
	// カメラは低い角度から見ているので、寝かせたままだと遠くのノーツは真横から見た細い線になる
	UPROPERTY(EditDefaultsOnly, Category="Note|Shape", meta=(ClampMin=-90.0, ClampMax=90.0, Units="deg"))
	float StartTilt = 60.0f;

	// 経過時間の割合 T（0〜）での板の傾き（度）。T = 0 で StartTilt、T >= 1 で PlateTilt
	float CalcTilt(float T) const;

	// ---------- 画像のノーツ（SetNoteTexture したとき） ----------

	// 画像を貼るマテリアル。初期値は Paper2D の TranslucentUnlitSpriteMaterial（透明部分を抜いて光らせずにそのまま表示）
	UPROPERTY(EditDefaultsOnly, Category="Note|Texture")
	TObjectPtr<UMaterialInterface> NoteTextureMaterial;

	// 上のマテリアルの、画像を入れる Texture パラメータ名（TranslucentUnlitSpriteMaterial は SpriteTexture）
	UPROPERTY(EditDefaultsOnly, Category="Note|Texture")
	FName NoteTextureParameterName = TEXT("SpriteTexture");

	// 画像の横幅のうち、六角形が占める割合。六角形の幅が PlateWidth になるように板を大きくする
	UPROPERTY(EditDefaultsOnly, Category="Note|Texture", meta=(ClampMin=0.01, ClampMax=1.0))
	float TextureHexWidthRatio = 0.57f;

	// 画像の高さのうち、六角形が占める割合（判定枠の奥行きに使う）
	UPROPERTY(EditDefaultsOnly, Category="Note|Texture", meta=(ClampMin=0.01, ClampMax=1.0))
	float TextureHexHeightRatio = 0.802f;

	// 画像のノーツの厚み（cm）。画像の六角形の形に沿って、レーン色の側面をつける（0 で厚みなし）
	UPROPERTY(EditDefaultsOnly, Category="Note|Texture", meta=(ClampMin=0.0, Units="cm"))
	float TextureThickness = 8.0f;

	// 画像の板を作る（ホールドは終点にも作るので上書きする）
	virtual void SetupTextureVisual();

	// 画像の板を Target に作って、画像を貼る
	void BuildTexturePlate(UProceduralMeshComponent* Target) const;

	// 判定地点を通り過ぎてから消えるまでの割合（1.0 = 判定地点、1.3 = 少し通り過ぎた所）
	UPROPERTY(EditAnywhere, Category="Note", meta=(ClampMin=1.0))
	float DestroyProgress = 1.3f;

	// 経過時間の割合 T から位置・消滅を更新する（ホールドは見た目が違うので上書きする）
	virtual void UpdateNote(float T);

	// 色を反映する（ホールドは複数のメッシュに塗るので上書きする）
	virtual void ApplyColor(const FLinearColor& Color);

	// 経過時間の割合 T（0〜）から位置を計算する
	FVector CalcPosition(float T) const;

	// 経過時間の割合 T（0〜）から向きを計算する（T = 0 で出現時の向き、T >= 1 で ArrivalYaw まで回った向き）
	FQuat CalcRotation(float T) const;

	// 見た目の準備：傾きをかけ、モデルがあればモデル、無ければ仮のプレートを表示する
	void SetupVisual(USceneComponent* Pivot, UProceduralMeshComponent* InPlate, UStaticMeshComponent* InMesh) const;

	// 見た目のコンポーネントに色を塗る（表示中のほうだけ）
	static void SetMeshColor(UMeshComponent* Target, const FLinearColor& Color);

	float TravelTime = 1.0f;

	int32 LaneIndex = 0;

	// ノーツの大きさ（倍率）
	float NoteScale = 1.0f;

	// ノーツの画像（空なら六角形の板）
	UPROPERTY()
	TObjectPtr<UTexture2D> NoteTexture;

	// 右レーン用の画像か（六角形の形が左右反転）
	bool bTextureRightSide = false;

	bool bWaitForJudge = false;

private:

	FVector StartPos = FVector::ZeroVector;
	FVector TargetPos = FVector::ZeroVector;
	float ArcHeight = 0.0f;

	// 出現時の向きと、判定地点に届いたときのヨー回転（度）
	FQuat StartRotation = FQuat::Identity;
	float ArrivalYaw = 0.0f;

	// 出現した時刻（ワールド時間)
	double SpawnTime = 0.0;

	bool bLaunched = false;
};
