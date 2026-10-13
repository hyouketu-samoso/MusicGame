// 音ゲーの調整値（難易度調整仕様書「調整値一覧」の管理先）。
// エディタの「プロジェクト設定 → ゲーム → Rhythm Game」で編集でき、Config/DefaultGame.ini に保存される。
// プレイテストで値を変えるときはここだけ直せばよい（コードの変更は不要）。

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RhythmGameSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Rhythm Game"))
class URhythmGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	URhythmGameSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// ノーツ速度 1〜5 の到達時間（秒）。要素0 = 速度1 … 要素4 = 速度5。速度が上がるほど短くする
	// ノーツが出現してから判定地点に届くまでの時間（ADJ-001〜005）
	UPROPERTY(Config, EditAnywhere, Category="Note Speed", meta=(ClampMin=0.1, EditFixedSize))
	TArray<float> SpeedArrivalTimes;

	// ゲーム開始時の速度（初期選択：速度3）
	UPROPERTY(Config, EditAnywhere, Category="Note Speed", meta=(ClampMin=1, ClampMax=5))
	int32 DefaultSpeedLevel = 3;

	// 速度の段階数（速度1〜5）
	static constexpr int32 MinSpeedLevel = 1;
	static constexpr int32 MaxSpeedLevel = 5;

	// 指定した速度の到達時間（範囲外の速度は 1〜5 に収める）
	float GetArrivalTime(int32 SpeedLevel) const;
};
