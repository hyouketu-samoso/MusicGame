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

	// 判定地点を通り過ぎてから消えるまでの割合（1.0 = 判定地点、1.3 = 少し通り過ぎた所）
	UPROPERTY(EditAnywhere, Category="Note", meta=(ClampMin=1.0))
	float DestroyProgress = 1.3f;

	// 経過時間の割合 T から位置・消滅を更新する（ホールドは見た目が違うので上書きする）
	virtual void UpdateNote(float T);

	// 色を反映する（ホールドは複数のメッシュに塗るので上書きする）
	virtual void ApplyColor(const FLinearColor& Color);

	// 経過時間の割合 T（0〜）から位置を計算する
	FVector CalcPosition(float T) const;

	// 見た目の準備：傾きをかけ、モデルがあればモデル、無ければ仮のプレートを表示する
	void SetupVisual(USceneComponent* Pivot, UProceduralMeshComponent* InPlate, UStaticMeshComponent* InMesh) const;

	// 見た目のコンポーネントに色を塗る（表示中のほうだけ）
	static void SetMeshColor(UMeshComponent* Target, const FLinearColor& Color);

	float TravelTime = 1.0f;

	int32 LaneIndex = 0;

	bool bWaitForJudge = false;

private:

	FVector StartPos = FVector::ZeroVector;
	FVector TargetPos = FVector::ZeroVector;
	float ArcHeight = 0.0f;

	// 出現した時刻（ワールド時間)
	double SpawnTime = 0.0;

	bool bLaunched = false;
};
