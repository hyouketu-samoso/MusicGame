// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMainWidget.h"
#include "MediaPlayer.h"
#include "MediaSource.h"

void UGameMainWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PlayVideos();


}

void UGameMainWidget::PlayVideos()
{
	if (Player1 && Source1)
	{
		Player1->SetLooping(true);
		Player1->OpenSource(Source1);
	}
	if (Player2 && Source2)
	{
		Player2->SetLooping(true);
		Player2->OpenSource(Source2);
	}
}

void UGameMainWidget::StopVideos()
{
	if (Player1) { Player1->Close(); }
	if (Player2) { Player2->Close(); }
}
