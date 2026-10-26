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
#include "Sound/SoundClass.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

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
		Effects.EmptyPress = Load(N.JudgementFolder, Prefix + TEXT("Dry"));  // 空打ち（NS_notes_effect_xxx_Dry）
	}

	// エフェクトは Niagara のプレビュー（+X 側から −X 向きに見る）に合わせて作られている。
	// ゲームのカメラは NoteSpawner の前方（+X）を向くので、180 度回してプレビューと同じ見た目にする
	// （エミッターが Local Space のときだけ回転が効く）
	EffectRotation = FRotator(0.0f, 180.0f, 0.0f);

	// ※ 内側2レーン（赤暗・青暗）はエフェクトが内側へ飛ぶ。bFlipSide では矢印が逆向きになるため使わず、
	//    デザイナーに外側向きのアセットを依頼中（届いたら差し替える）

	// 外側2レーンはアセット自体も斜め（10/19「ノーツ角度調整」）だが、判定枠に合わせてさらに画面上で傾ける
	LaneEffects[0].ScreenTilt = -35.0f;  // 左外側：左が上がる「＼」
	LaneEffects[3].ScreenTilt = 35.0f;   // 右外側：右が上がる「／」

	// ---------- SE の Sound Class（ポーズ画面の SE 音量スライダーが効く） ----------
	static ConstructorHelpers::FObjectFinder<USoundClass> SEClassFinder(TEXT("/Game/MusicGame/Sound_P/Sound_Goto/SCL_SE.SCL_SE"));
	SoundClass = SEClassFinder.Object;

	// ---------- サウンド担当の SE（Content/MusicGame/Sound_P/Sound_SE）を初期値にする ----------
	// Miss の SE は無いので鳴らさない。ホールドの終点は JudgementSounds と同じ音を使う
	auto LoadSound = [](const TCHAR* Name) -> USoundBase*
	{
		const FString Path = FString::Printf(TEXT("/Game/MusicGame/Sound_P/Sound_SE/%s.%s"), Name, Name);
		ConstructorHelpers::FObjectFinder<USoundBase> Finder(*Path);
		return Finder.Object;
	};

	// 判定 SE（_1019 は頭の無音を削った版なので、そのまま頭から鳴らす）
	JudgementSounds.Perfect = LoadSound(TEXT("SE_Notes_perfect_1019"));
	JudgementSounds.Great   = LoadSound(TEXT("SE_Notes_great_1019"));
	JudgementSounds.Good    = LoadSound(TEXT("SE_Notes_good_1019"));
	JudgementSounds.Bad     = LoadSound(TEXT("SE_Notes_bad_1019"));

	// ホールドを押している間に鳴らす（約 9.5 秒。ホールドより長いので、離したところで止める）
	HoldLoopSound = LoadSound(TEXT("SE_Long_Notes"));

	// 空打ち
	EmptyPressSound = LoadSound(TEXT("SE_Notes_karauti_1019"));
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
		Judge->OnLaneEmptyPress.RemoveDynamic(this, &URhythmFeedbackComponent::HandleLaneEmptyPress);
		Judge->GetScoreComponent()->OnGameOver.RemoveDynamic(this, &URhythmFeedbackComponent::HandleGameOver);
	}

	Judge = InJudge;
	Judge->OnNoteJudged.AddDynamic(this, &URhythmFeedbackComponent::HandleNoteJudged);
	Judge->OnLaneEmptyPress.AddDynamic(this, &URhythmFeedbackComponent::HandleLaneEmptyPress);
	Judge->GetScoreComponent()->OnGameOver.AddDynamic(this, &URhythmFeedbackComponent::HandleGameOver);
}

void URhythmFeedbackComponent::HandleLaneEmptyPress(int32 LaneIndex)
{
	// ミスしてから MissPressWindow 秒以内の入力なら、空打ちの代わりにミスのエフェクト（最初の1回だけ）
	double Deadline = 0.0;
	if (MissPressDeadlines.RemoveAndCopyValue(LaneIndex, Deadline) && GetWorld()->GetTimeSeconds() <= Deadline)
	{
		PlayMissPress(LaneIndex);
		return;
	}

	PlayEmptyPress(LaneIndex);
}

void URhythmFeedbackComponent::PlayMissPress(int32 LaneIndex)
{
	if (const FRhythmLaneEffects* Effects = FindLaneEffects(LaneIndex))
	{
		SpawnEffect(Effects->Judgement.Miss, LaneIndex);
	}

	PlaySound(MissPressSound);
}

void URhythmFeedbackComponent::PlayEmptyPress(int32 LaneIndex)
{
	if (const FRhythmLaneEffects* Effects = FindLaneEffects(LaneIndex))
	{
		SpawnEffect(Effects->EmptyPress, LaneIndex);
	}

	PlaySound(EmptyPressSound);
}

void URhythmFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初めて鳴らすときの読み込み待ちで遅れないよう、SE を先に読み込んでおく
	const FRhythmJudgementSounds* SoundSets[] = { &JudgementSounds, &HoldEndSounds };
	for (const FRhythmJudgementSounds* Set : SoundSets)
	{
		for (USoundBase* Sound : { Set->Perfect, Set->Great, Set->Good, Set->Bad, Set->Miss })
		{
			if (Sound)
			{
				UGameplayStatics::PrimeSound(Sound);
			}
		}
	}
	for (USoundBase* Sound : { HoldLoopSound.Get(), EmptyPressSound.Get() })
	{
		if (Sound)
		{
			UGameplayStatics::PrimeSound(Sound);
		}
	}
}

void URhythmFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllHolds();

	Super::EndPlay(EndPlayReason);
}

void URhythmFeedbackComponent::HandleNoteJudged(ERhythmJudgement Judgement, int32 LaneIndex, float TimingError, int32 Combo, ENoteJudgePoint Point)
{
	// ミスの瞬間は何も出さない。見逃しただけならエフェクトなし
	if (Judgement == ERhythmJudgement::Miss)
	{
		if (Point == ENoteJudgePoint::HoldEnd)
		{
			// 終点のミス（早く離した・離し遅れ）は押した場面ではないので、受付も始めない
			StopHold(LaneIndex);
		}
		else
		{
			// 通常ノーツ・始点のミス：MissPressWindow 秒以内にこのレーンを押したら、ミスのエフェクトを出す
			MissPressDeadlines.Add(LaneIndex, GetWorld()->GetTimeSeconds() + MissPressWindow);
		}
		return;
	}

	const FRhythmLaneEffects* Effects = FindLaneEffects(LaneIndex);

	// 判定エフェクト。ホールドの始点・終点はロングノーツ用のエフェクト（charge / burst）だけにする（設定で戻せる）
	const bool bHoldPoint = (Point != ENoteJudgePoint::Tap);
	if (Effects && (!bHoldPoint || bShowJudgementEffectOnHold))
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

		if (USoundBase* EndSound = HoldEndSounds.Get(Judgement))
		{
			PlaySound(EndSound);
		}
		else
		{
			PlaySound(JudgementSounds.Get(Judgement));
		}
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
		// 押している間は出し続けるので、時間の上限は付けない（StopHold で消す）
		if (UNiagaraComponent* Effect = SpawnEffect(Effects->HoldCharge, LaneIndex, /*MaxLifetime=*/0.0f))
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
		// 出ている粒ごとすぐに消す（Deactivate だと寿命が無限の粒が判定ラインに残り続ける）
		Effect->DeactivateImmediate();
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

UNiagaraComponent* URhythmFeedbackComponent::SpawnEffect(UNiagaraSystem* System, int32 LaneIndex)
{
	return SpawnEffect(System, LaneIndex, EffectMaxLifetime);
}

UNiagaraComponent* URhythmFeedbackComponent::SpawnEffect(UNiagaraSystem* System, int32 LaneIndex, float MaxLifetime)
{
	if (!System)
	{
		return nullptr;
	}

	const FTransform Transform = GetEffectTransform(LaneIndex);

	UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this, System, Transform.GetLocation(), Transform.Rotator(), Transform.GetScale3D(), /*bAutoDestroy=*/true);

	// アセットがループ設定のままでも残り続けないよう、上限時間で消す（消すと自動で破棄される）
	if (Effect && MaxLifetime > 0.0f)
	{
		TWeakObjectPtr<UNiagaraComponent> WeakEffect = Effect;
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, [WeakEffect]()
		{
			if (WeakEffect.IsValid())
			{
				WeakEffect->DeactivateImmediate();
			}
		}, MaxLifetime, false);
	}

	return Effect;
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
	FQuat Rotation = EffectRotation.Quaternion();
	if (const FRhythmLaneEffects* Effects = FindLaneEffects(LaneIndex))
	{
		Offset += Effects->LaneOffset;

		// 画面上の傾き：エフェクトの X 軸（カメラの方向）を軸に傾ける
		Rotation = Rotation * FRotator(0.0f, 0.0f, Effects->ScreenTilt).Quaternion();

		// 左右を逆にする：エフェクトの X 軸（カメラの方向）を軸に 180 度回す
		if (Effects->bFlipSide)
		{
			Rotation = Rotation * FRotator(0.0f, 0.0f, 180.0f).Quaternion();
		}
	}

	const FQuat OwnerQuat = Owner->GetActorQuat();
	return FTransform(
		OwnerQuat * Rotation,
		LanePos + OwnerQuat.RotateVector(Offset),
		EffectScale);
}

const FRhythmLaneEffects* URhythmFeedbackComponent::FindLaneEffects(int32 LaneIndex) const
{
	return LaneEffects.IsValidIndex(LaneIndex) ? &LaneEffects[LaneIndex] : nullptr;
}
