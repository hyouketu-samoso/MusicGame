#include "RhythmFeedbackComponent.h"
#include "RhythmJudge.h"
#include "RhythmScoreComponent.h"
#include "NoteSpawner.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

URhythmFeedbackComponent::URhythmFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// ---------- デザイナーのエフェクト（Content/Designer/Effect/Niagara_NS）を初期値にする ----------
	// 左外側 = 赤白 / 左内側 = 赤暗 / 右内側 = 青暗 / 右外側 = 青白（青白は Blue フォルダが最新）
	// ミスは左レーン = NS_notes_effect_Miss、右レーン = NS_notes_effect_Miss_right

	auto Load = [](const FString& Folder, const FString& Name) -> UNiagaraSystem*
	{
		const FString Path = FString::Printf(TEXT("/Game/Designer/Effect/Niagara_NS/%s%s.%s"), *Folder, *Name, *Name);
		ConstructorHelpers::FObjectFinder<UNiagaraSystem> Finder(*Path);
		return Finder.Object;
	};

	struct FLaneEffectNames
	{
		const TCHAR* JudgementFolder;  // 判定エフェクトのフォルダ（"" = Niagara_NS 直下）
		const TCHAR* JudgementPrefix;  // NS_notes_effect_<ここ>_perfect など
		const TCHAR* HoldFolder;
		const TCHAR* HoldName;         // NS_hold_<ここ>_charge / _burst
		const TCHAR* Miss;
	};

	static const FLaneEffectNames Names[] =
	{
		{ TEXT(""),           TEXT("Red_white"),  TEXT(""),           TEXT("white_Red"), TEXT("NS_notes_effect_Miss") },
		{ TEXT("Red_dark/"),  TEXT("Red_dark"),   TEXT("Red_dark/"),  TEXT("dark_Red"),  TEXT("NS_notes_effect_Miss") },
		{ TEXT("Blue_dark/"), TEXT("Blue_dark"),  TEXT("Blue_dark/"), TEXT("Blue_dark"), TEXT("NS_notes_effect_Miss_right") },
		// ※ Blue/NS_notes_effect_Blue_white_* は Blue_dark へのリダイレクタ（中身なし）。青白の本体は Blue/NS_notes_effect_Blue_*
		{ TEXT("Blue/"),      TEXT("Blue"),       TEXT("Blue/"),      TEXT("Blue"),      TEXT("NS_notes_effect_Miss_right") },
	};

	LaneEffects.SetNum(UE_ARRAY_COUNT(Names));
	for (int32 Lane = 0; Lane < UE_ARRAY_COUNT(Names); ++Lane)
	{
		const FLaneEffectNames& N = Names[Lane];
		const FString Prefix = FString::Printf(TEXT("NS_notes_effect_%s_"), N.JudgementPrefix);
		const FString Hold = FString::Printf(TEXT("NS_hold_%s_"), N.HoldName);

		FRhythmLaneEffects& Effects = LaneEffects[Lane];
		Effects.Judgement.Perfect = Load(N.JudgementFolder, Prefix + TEXT("perfect"));
		Effects.Judgement.Great   = Load(N.JudgementFolder, Prefix + TEXT("Great"));
		Effects.Judgement.Good    = Load(N.JudgementFolder, Prefix + TEXT("good"));
		Effects.Judgement.Bad     = Load(N.JudgementFolder, Prefix + TEXT("Bad"));
		Effects.Judgement.Miss    = Load(TEXT(""), N.Miss);
		Effects.HoldCharge = Load(N.HoldFolder, Hold + TEXT("charge"));
		Effects.HoldBurst  = Load(N.HoldFolder, Hold + TEXT("burst"));
	}

	// 左外側だけエフェクトが内側（右）へ出るので、左右反転して外側へ出す
	LaneEffects[0].bMirror = true;
}

void URhythmFeedbackComponent::BindJudge(ARhythmJudge* InJudge)
{
	if (Judge == InJudge || !IsValid(InJudge))
	{
		return;
	}

	if (IsValid(Judge))
	{
		Judge->OnNoteJudged.RemoveDynamic(this, &URhythmFeedbackComponent::HandleNoteJudged);
		Judge->GetScoreComponent()->OnGameOver.RemoveDynamic(this, &URhythmFeedbackComponent::HandleGameOver);
	}

	Judge = InJudge;
	Judge->OnNoteJudged.AddDynamic(this, &URhythmFeedbackComponent::HandleNoteJudged);
	Judge->GetScoreComponent()->OnGameOver.AddDynamic(this, &URhythmFeedbackComponent::HandleGameOver);
}

void URhythmFeedbackComponent::PlayEmptyPress(int32 LaneIndex)
{
	if (const FRhythmLaneEffects* Effects = FindLaneEffects(LaneIndex))
	{
		SpawnEffect(Effects->EmptyPress, LaneIndex);
	}

	PlaySound(EmptyPressSound);
}

void URhythmFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllHolds();

	Super::EndPlay(EndPlayReason);
}

void URhythmFeedbackComponent::HandleNoteJudged(ERhythmJudgement Judgement, int32 LaneIndex, float TimingError, int32 Combo, ENoteJudgePoint Point)
{
	const FRhythmLaneEffects* Effects = FindLaneEffects(LaneIndex);

	// 判定エフェクト（通常・始点・終点共通）
	if (Effects)
	{
		SpawnEffect(Effects->Judgement.Get(Judgement), LaneIndex);
	}

	switch (Point)
	{
	case ENoteJudgePoint::Tap:
		PlaySound(JudgementSounds.Get(Judgement));
		break;

	case ENoteJudgePoint::HoldStart:
		PlaySound(JudgementSounds.Get(Judgement));

		// 始点ミスは押し続けにならない。始点の Bad でゲームオーバーになったときも始めない
		// （ゲームオーバーの通知はこの判定の通知より先に来ている）
		if (Judgement != ERhythmJudgement::Miss && !(IsValid(Judge) && Judge->IsGameOver()))
		{
			StartHold(LaneIndex);
		}
		break;

	case ENoteJudgePoint::HoldEnd:
	{
		StopHold(LaneIndex);

		// burst は Good 以上のときだけ（Bad・ミスでは出さない）
		if (Effects && KeepsCombo(Judgement))
		{
			SpawnEffect(Effects->HoldBurst, LaneIndex);
		}

		USoundBase* EndSound = HoldEndSounds.Get(Judgement);
		PlaySound(EndSound ? EndSound : JudgementSounds.Get(Judgement));
		break;
	}
	}
}

void URhythmFeedbackComponent::HandleGameOver()
{
	// ゲームオーバーでホールドは手放され、終点の判定は来ない
	StopAllHolds();
}

void URhythmFeedbackComponent::StartHold(int32 LaneIndex)
{
	StopHold(LaneIndex);

	if (const FRhythmLaneEffects* Effects = FindLaneEffects(LaneIndex))
	{
		if (UNiagaraComponent* Effect = SpawnEffect(Effects->HoldCharge, LaneIndex))
		{
			HoldEffects.Add(LaneIndex, Effect);
		}
	}

	if (UAudioComponent* Sound = StartSound(HoldLoopSound))
	{
		HoldSounds.Add(LaneIndex, Sound);
	}
}

void URhythmFeedbackComponent::StopHold(int32 LaneIndex)
{
	TObjectPtr<UNiagaraComponent> Effect;
	if (HoldEffects.RemoveAndCopyValue(LaneIndex, Effect) && IsValid(Effect))
	{
		// 新しい粒の発生だけ止める（出ている粒は消え切ってから自動で破棄される）
		Effect->Deactivate();
	}

	TObjectPtr<UAudioComponent> Sound;
	if (HoldSounds.RemoveAndCopyValue(LaneIndex, Sound) && IsValid(Sound))
	{
		Sound->Stop();
	}
}

void URhythmFeedbackComponent::StopAllHolds()
{
	TSet<int32> Lanes;
	for (const TPair<int32, TObjectPtr<UNiagaraComponent>>& Pair : HoldEffects) { Lanes.Add(Pair.Key); }
	for (const TPair<int32, TObjectPtr<UAudioComponent>>& Pair : HoldSounds)    { Lanes.Add(Pair.Key); }

	for (int32 Lane : Lanes)
	{
		StopHold(Lane);
	}
}

UNiagaraComponent* URhythmFeedbackComponent::SpawnEffect(UNiagaraSystem* System, int32 LaneIndex) const
{
	if (!System)
	{
		return nullptr;
	}

	const FTransform Transform = GetEffectTransform(LaneIndex);

	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this, System, Transform.GetLocation(), Transform.Rotator(), Transform.GetScale3D(), /*bAutoDestroy=*/true);
}

void URhythmFeedbackComponent::PlaySound(USoundBase* Sound) const
{
	StartSound(Sound);
}

UAudioComponent* URhythmFeedbackComponent::StartSound(USoundBase* Sound) const
{
	if (!Sound)
	{
		return nullptr;
	}

	// 鳴り終わったら自動で破棄される
	UAudioComponent* Audio = UGameplayStatics::CreateSound2D(this, Sound, SoundVolume);
	if (!Audio)
	{
		return nullptr;
	}

	if (SoundClass)
	{
		Audio->SoundClassOverride = SoundClass;
	}
	Audio->Play();
	return Audio;
}

FTransform URhythmFeedbackComponent::GetEffectTransform(int32 LaneIndex) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FTransform::Identity;
	}

	// 位置は NoteSpawner の判定ライン（レーンごと）。NoteSpawner 以外に付けたときは持ち主の位置
	const ANoteSpawner* Spawner = Cast<ANoteSpawner>(Owner);
	const FVector LanePos = Spawner ? Spawner->GetLaneTarget(LaneIndex) : Owner->GetActorLocation();

	FVector Offset = EffectOffset;
	FVector Scale = EffectScale;
	if (const FRhythmLaneEffects* Effects = FindLaneEffects(LaneIndex))
	{
		Offset += Effects->LaneOffset;

		// 左右反転（エフェクトの Y 軸を裏返す）
		if (Effects->bMirror)
		{
			Scale.Y = -Scale.Y;
		}
	}

	const FQuat OwnerQuat = Owner->GetActorQuat();
	return FTransform(
		OwnerQuat * EffectRotation.Quaternion(),
		LanePos + OwnerQuat.RotateVector(Offset),
		Scale);
}

const FRhythmLaneEffects* URhythmFeedbackComponent::FindLaneEffects(int32 LaneIndex) const
{
	return LaneEffects.IsValidIndex(LaneIndex) ? &LaneEffects[LaneIndex] : nullptr;
}
