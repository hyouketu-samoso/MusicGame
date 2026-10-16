// 判定の演出（エフェクト・SE）を出すコンポーネント。NoteSpawner に付いている。
// 判定役（ARhythmJudge）の通知を受けて、判定ラインのレーン位置にエフェクトを出し、SE を鳴らす。
//
// ・通常ノーツ / ホールドの始点 : 判定ごとのエフェクト + SE
// ・ホールドを押している間     : HoldCharge エフェクト（+ HoldLoopSound）を出し続ける
// ・ホールドの終点（離したとき）: HoldCharge を止めて、判定ごとのエフェクト + HoldBurst（Good 以上）+ SE
// ・空打ち（ノーツが無いところを押した）: EmptyPress エフェクト + EmptyPressSound
//
// エフェクトはデザイナーのアセットが初期値（左外側 = 赤白 / 左内側 = 赤暗 / 右内側 = 青暗 / 右外側 = 青白）。向きは Yaw 180（Niagara のプレビューと同じ見た目）。内側2レーンは外側向きのアセット待ち。
// SE はサウンド担当の SE（Sound_P/Sound_SE）が初期値。どちらもエディタで差し替えられる（空の欄は何も出さない）。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RhythmTypes.h"
#include "RhythmFeedbackComponent.generated.h"

class ARhythmJudge;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class UAudioComponent;
class USoundClass;

// 判定ごとのエフェクト
USTRUCT(BlueprintType)
struct FRhythmJudgementEffects
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<UNiagaraSystem> Perfect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<UNiagaraSystem> Great = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<UNiagaraSystem> Good = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<UNiagaraSystem> Bad = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<UNiagaraSystem> Miss = nullptr;

	UNiagaraSystem* Get(ERhythmJudgement Judgement) const
	{
		switch (Judgement)
		{
		case ERhythmJudgement::Perfect: return Perfect;
		case ERhythmJudgement::Great:   return Great;
		case ERhythmJudgement::Good:    return Good;
		case ERhythmJudgement::Bad:     return Bad;
		default:                        return Miss;
		}
	}
};

// 1レーン分のエフェクト一式
USTRUCT(BlueprintType)
struct FRhythmLaneEffects
{
	GENERATED_BODY()

	// 通常ノーツ・ホールドの始点と終点の判定エフェクト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	FRhythmJudgementEffects Judgement;

	// ホールドを押している間に出し続けるエフェクト（NS_hold_xxx_charge）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<UNiagaraSystem> HoldCharge = nullptr;

	// ホールドの終点で離したときのエフェクト（NS_hold_xxx_burst）。終点が Bad・ミスのときは出さない
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<UNiagaraSystem> HoldBurst = nullptr;

	// 空打ち（ノーツが無いところを押した）のエフェクト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<UNiagaraSystem> EmptyPress = nullptr;

	// このレーンのエフェクトの左右を逆にする（内側へ出るエフェクトを外側へ向けるときなど）。
	// 裏返す（拡大率をマイナスにする）と板の表裏も入れ替わって見えなくなるので、
	// カメラの方向を軸に 180 度回す。そのため上下も逆になる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	bool bFlipSide = false;

	// 画面上の傾き（度）。エフェクトはカメラの方を向いた板なので、カメラの方向を軸に傾けて
	// 斜めの判定枠（NoteSpawner の LaneTargetYaws）と同じ向きに見せる。マイナスで左側が上がる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm", meta=(Units="deg"))
	float ScreenTilt = 0.0f;

	// このレーンだけのずらし（EffectOffset に足す。NoteSpawner の向き基準、cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm", meta=(Units="cm"))
	FVector LaneOffset = FVector::ZeroVector;
};

// 判定ごとの SE
USTRUCT(BlueprintType)
struct FRhythmJudgementSounds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<USoundBase> Perfect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<USoundBase> Great = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<USoundBase> Good = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<USoundBase> Bad = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm")
	TObjectPtr<USoundBase> Miss = nullptr;

	// 再生を始める位置（秒）。SE の頭に無音があると押してから遅れて聞こえるので、その分を飛ばす
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm", meta=(ClampMin=0.0, Units="s"))
	float StartTime = 0.0f;

	USoundBase* Get(ERhythmJudgement Judgement) const
	{
		switch (Judgement)
		{
		case ERhythmJudgement::Perfect: return Perfect;
		case ERhythmJudgement::Great:   return Great;
		case ERhythmJudgement::Good:    return Good;
		case ERhythmJudgement::Bad:     return Bad;
		default:                        return Miss;
		}
	}
};

UCLASS(ClassGroup=(Rhythm), meta=(BlueprintSpawnableComponent))
class URhythmFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	URhythmFeedbackComponent();

	// 判定役の通知に登録する（NoteSpawner が判定役を見つけたあとに呼ぶ）
	void BindJudge(ARhythmJudge* InJudge);

	// 出しているホールドのエフェクト・ループ音をすべて止める
	UFUNCTION(BlueprintCallable, Category="Rhythm Feedback")
	void StopAllHolds();

	// 空打ち（ノーツが無いところを押した）の演出を出す。
	// ※ 空打ちの通知はまだ無い。判定役に通知ができたら、そこから呼ぶ（または BindJudge で登録する）
	UFUNCTION(BlueprintCallable, Category="Rhythm Feedback")
	void PlayEmptyPress(int32 LaneIndex);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ---------- エフェクト ----------

	// レーンごとのエフェクト（要素番号 = レーン番号。0=左外側 / 1=左内側 / 2=右内側 / 3=右外側）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Effect")
	TArray<FRhythmLaneEffects> LaneEffects;

	// 判定ラインのレーン位置からのずらし（NoteSpawner の向き基準、cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Effect", meta=(Units="cm"))
	FVector EffectOffset = FVector::ZeroVector;

	// エフェクトの向き（NoteSpawner の向き基準）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Effect")
	FRotator EffectRotation = FRotator::ZeroRotator;

	// エフェクトの大きさ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Effect")
	FVector EffectScale = FVector::OneVector;

	// 使い捨てのエフェクト（判定・burst・空打ち）を強制的に消すまでの時間（0 なら消さない）。
	// アセットがループ設定のままでも、判定ラインに残り続けないようにする
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Effect", meta=(ClampMin=0.0, Units="s"))
	float EffectMaxLifetime = 2.0f;

	// ---------- SE ----------

	// 通常ノーツ・ホールドの始点の判定 SE
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Sound")
	FRhythmJudgementSounds JudgementSounds;

	// ホールドの終点の判定 SE（空の欄は JudgementSounds の同じ判定を使う）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Sound")
	FRhythmJudgementSounds HoldEndSounds;

	// ホールドを押している間に鳴らし続ける音（ループ設定の音を入れる）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Sound")
	TObjectPtr<USoundBase> HoldLoopSound = nullptr;

	// 空打ち（ノーツが無いところを押した）の SE
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Sound")
	TObjectPtr<USoundBase> EmptyPressSound = nullptr;

	// SE 全体の音量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Sound", meta=(ClampMin=0.0))
	float SoundVolume = 1.0f;

	// SE に使う Sound Class（ポーズ画面の SE 音量スライダーが効くように SCL_SE を入れる）。
	// 空のときは各 SE アセットに設定された Sound Class のまま鳴らす
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm Feedback|Sound")
	TObjectPtr<USoundClass> SoundClass = nullptr;

private:

	UFUNCTION()
	void HandleNoteJudged(ERhythmJudgement Judgement, int32 LaneIndex, float TimingError, int32 Combo, ENoteJudgePoint Point);

	UFUNCTION()
	void HandleGameOver();

	// ホールドを押し始めた / 離した（終点の判定が出た）
	void StartHold(int32 LaneIndex);
	void StopHold(int32 LaneIndex);

	// レーン位置にエフェクトを出す（使い捨て）
	UNiagaraComponent* SpawnEffect(UNiagaraSystem* System, int32 LaneIndex);

	// MaxLifetime 秒たったら消す（0 以下なら消さない）
	UNiagaraComponent* SpawnEffect(UNiagaraSystem* System, int32 LaneIndex, float MaxLifetime);

	void PlaySound(USoundBase* Sound, float StartTime = 0.0f) const;

	// SE を鳴らす（SoundClass を反映）。止めたいとき用に AudioComponent を返す
	UAudioComponent* StartSound(USoundBase* Sound, float StartTime = 0.0f) const;

	// エフェクトを出す位置と向き
	FTransform GetEffectTransform(int32 LaneIndex) const;

	const FRhythmLaneEffects* FindLaneEffects(int32 LaneIndex) const;

	UPROPERTY()
	TObjectPtr<ARhythmJudge> Judge;

	// ホールド中のエフェクト・ループ音（キー = レーン番号）
	UPROPERTY()
	TMap<int32, TObjectPtr<UNiagaraComponent>> HoldEffects;

	UPROPERTY()
	TMap<int32, TObjectPtr<UAudioComponent>> HoldSounds;
};
