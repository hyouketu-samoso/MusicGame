// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "BackVideoActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMediaPlayer;
class UMediaSource;
class UMediaTexture;

USTRUCT(BlueprintType)
struct FBackVideoSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMediaPlayer> Player;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMediaSource> Source;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMediaTexture> Texture;
};

UCLASS()
class MUSICGAME_API ABackVideoActor : public AActor
{
	GENERATED_BODY()

public:
	ABackVideoActor();

	// 板。Detailsの Rotation / Location / Scale で、角度や位置を数値で調整できる
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UStaticMeshComponent> Video1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UStaticMeshComponent> Video2;

	// 3D用のマテリアル（M_VideoWorld）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	FBackVideoSlot Slot1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	FBackVideoSlot Slot2;

	// リトライ時などに、動画をこの秒数から再生し直す(冒頭のフェードインを飛ばすため)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video", meta = (ClampMin = "0.0", Units = "s"))
	float RestartSeconds = 1.5f;

	// 動画のループをさせない。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video")
	bool bLoopVideo = false;

	// ポーズ画面に入った・出た時に動画を止める/再開する
	void PauseVideos();
	void ResumeVideos();

	// タイトルからゲームを始めるとき: タイトルで止めておいた動画を再生する
	void ReleaseFromTitle();

	// タイトルからゲームを始めるときの、動画の開始位置(秒)。リトライ用の RestartSeconds とは別
	// 0 なら動画の頭から。冒頭の黒(フェード)を短くしたいときは大きくする
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Video", meta = (ClampMin = "0.0", Units = "s"))
	float TitleStartSeconds = 1.3f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SetupSlot(UStaticMeshComponent* Mesh, const FBackVideoSlot& InSlot);

	// タイトルのとき: 動画を止めて 0 秒へ移動だけしておく(再生はしない)
	void ParkAtTitle(const FBackVideoSlot& InSlot);

	// リトライで開き直したレベルか
	bool IsRetryStart() const;

	// タイトルで待機中(止めて開始位置に置いてある)か。
	// ポーズの再開(ResumeVideos)で動かさないために使う。ReleaseFromTitle で下りる
	bool bParkedAtTitle = false;

	// タイトルで 0 秒へ移動している最中か(Slot1 / Slot2)
	bool bTitleSeekPending[2] = { false, false };

	// Seek が終わったら再生を始める
	UFUNCTION()
	void HandleSeekCompleted();

	// 移動待ちの動画を再生する(Seek の通知が来なかったときの保険にも使う)
	void PlayWaitingVideos();

	// Slot1 / Slot2 が Seek の完了待ちか
	bool bWaitSeek[2] = { false, false };

	// Seek の通知が来なくても、一定時間後に再生を始めるためのタイマー
	FTimerHandle SeekFallbackTimer;
};