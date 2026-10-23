#include "RhythmHUDSyncComponent.h"
#include "RhythmJudge.h"
#include "RhythmScoreComponent.h"
#include "GameMainWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

URhythmHUDSyncComponent::URhythmHUDSyncComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URhythmHUDSyncComponent::BindJudge(ARhythmJudge* InJudge)
{
	if (Judge == InJudge || !IsValid(InJudge))
	{
		return;
	}

	if (IsValid(Judge))
	{
		Judge->OnNoteJudged.RemoveDynamic(this, &URhythmHUDSyncComponent::HandleNoteJudged);
		Judge->GetScoreComponent()->OnHealthChanged.RemoveDynamic(this, &URhythmHUDSyncComponent::HandleHealthChanged);
	}

	Judge = InJudge;
	Judge->OnNoteJudged.AddDynamic(this, &URhythmHUDSyncComponent::HandleNoteJudged);
	Judge->GetScoreComponent()->OnHealthChanged.AddDynamic(this, &URhythmHUDSyncComponent::HandleHealthChanged);

	// UI がもう出ていれば、いまの値を渡しておく（まだなら、見つかった時点で渡す）
	GetWidget();
}

void URhythmHUDSyncComponent::RefreshAll()
{
	if (UGameMainWidget* Widget = GetWidget())
	{
		PushScoreAndCombo(Widget);
		PushHealth(Widget);
	}
}

void URhythmHUDSyncComponent::HandleNoteJudged(ERhythmJudgement Judgement, int32 LaneIndex, float TimingError, int32 Combo, ENoteJudgePoint Point)
{
	UGameMainWidget* Widget = GetWidget();
	if (!Widget)
	{
		return;
	}

	PushScoreAndCombo(Widget);

	if (bShowJudgementText)
	{
		// 判定の種類を UI 用に置き換える（並びは同じ）
		EJudgement UIJudgement = EJudgement::Miss;
		switch (Judgement)
		{
		case ERhythmJudgement::Perfect: UIJudgement = EJudgement::Perfect; break;
		case ERhythmJudgement::Great:   UIJudgement = EJudgement::Great;   break;
		case ERhythmJudgement::Good:    UIJudgement = EJudgement::Good;    break;
		case ERhythmJudgement::Bad:     UIJudgement = EJudgement::Bad;     break;
		default:                        UIJudgement = EJudgement::Miss;    break;
		}
		Widget->ShowJudgement(UIJudgement);
	}
}

void URhythmHUDSyncComponent::HandleHealthChanged(float NewHealth, float Delta)
{
	if (UGameMainWidget* Widget = GetWidget())
	{
		PushHealth(Widget);
	}
}

UGameMainWidget* URhythmHUDSyncComponent::GetWidget()
{
	if (CachedWidget.IsValid())
	{
		return CachedWidget.Get();
	}

	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Found, UGameMainWidget::StaticClass(), /*TopLevelOnly=*/false);
	if (Found.Num() == 0)
	{
		return nullptr;
	}

	UGameMainWidget* Widget = Cast<UGameMainWidget>(Found[0]);
	CachedWidget = Widget;

	// 見つけた最初の1回だけ、いまの値をまとめて渡す
	if (Widget)
	{
		PushScoreAndCombo(Widget);
		PushHealth(Widget);
	}
	return Widget;
}

void URhythmHUDSyncComponent::PushScoreAndCombo(UGameMainWidget* Widget) const
{
	if (const URhythmScoreComponent* Score = GetScore())
	{
		Widget->SetScore(static_cast<float>(Score->GetDisplayScore()));
		Widget->SetCombo(Score->GetCombo());
	}
}

void URhythmHUDSyncComponent::PushHealth(UGameMainWidget* Widget) const
{
	if (const URhythmScoreComponent* Score = GetScore())
	{
		Widget->SetHP(FMath::RoundToInt(Score->GetHealth()), FMath::RoundToInt(Score->GetMaxHealth()));
	}
}

URhythmScoreComponent* URhythmHUDSyncComponent::GetScore() const
{
	return IsValid(Judge) ? Judge->GetScoreComponent() : nullptr;
}
