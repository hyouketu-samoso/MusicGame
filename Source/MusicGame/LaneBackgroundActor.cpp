// Fill out your copyright notice in the Description page of Project Settings.

#include "LaneBackgroundActor.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"

ALaneBackgroundActor::ALaneBackgroundActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	WidgetComp->SetupAttachment(Root);

	// 3Dの世界の中の板として表示する(画面に貼り付けない)
	WidgetComp->SetWidgetSpace(EWidgetSpace::World);
	WidgetComp->SetBlendMode(EWidgetBlendMode::Transparent);   // 画像の透明部分を、透明のまま出す
	WidgetComp->SetTwoSided(true);
	WidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComp->SetCastShadow(false);
	WidgetComp->SetDrawAtDesiredSize(false);
	WidgetComp->SetPivot(FVector2D(0.5f, 0.5f));
}

// エディタで設定を変えたときにも、すぐ反映する
void ALaneBackgroundActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (WidgetComp)
	{
		WidgetComp->SetWidgetClass(LaneWidgetClass);
		WidgetComp->SetDrawSize(FVector2D(DrawSize.X, DrawSize.Y));
	}
}