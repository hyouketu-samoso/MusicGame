// プレイヤーが開始画面で選ぶ設定（ノーツ速度・難易度）を持つサブシステム。
// GameInstance に1つだけあり、レベルを移動しても値が残る（開始画面で設定 → プレイ画面で使う）。
// ノーツ速度は自動でセーブされ、次にゲームを起動したときにも引き継がれる。
//
// 難易度調整仕様書より：難易度・ノーツ速度・判定はそれぞれ独立。
//   ・難易度を変えてもノーツ速度は自動で変えない
//   ・ノーツ速度を変えても譜面・判定幅は変えない
//
// 使い方（Blueprint）：「Get RhythmSettingsSubsystem」ノードから各関数を呼ぶ。
// 使い方（C++）    ：GetGameInstance()->GetSubsystem<URhythmSettingsSubsystem>()

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "RhythmTypes.h"
#include "RhythmSettingsSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNoteSpeedLevelChanged, int32, NewSpeedLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDifficultyChanged, ERhythmDifficulty, NewDifficulty);

// セーブデータ（ノーツ速度だけを保存する）
// ※ 入力設定（デッドゾーン・感度・振動）のセーブデータ URhythmSettingsSaveGame（Input フォルダ）とは別物
UCLASS()
class URhythmPlayerSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	// ノーツ速度（1〜5）。0 は未保存
	UPROPERTY()
	int32 NoteSpeedLevel = 0;
};

UCLASS()
class URhythmSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ---- ノーツ速度（1〜5、初期値 3） ----

	// 今の速度（1〜5）
	UFUNCTION(BlueprintPure, Category="Rhythm Settings")
	int32 GetNoteSpeedLevel() const { return NoteSpeedLevel; }

	// 速度を設定する（1〜5 の範囲に収める）。変更はすぐにセーブされる
	UFUNCTION(BlueprintCallable, Category="Rhythm Settings")
	void SetNoteSpeedLevel(int32 NewLevel);

	// 今の速度に Delta を足す（「＋」「−」ボタン用。例：Delta = 1 / −1）
	UFUNCTION(BlueprintCallable, Category="Rhythm Settings")
	void AddNoteSpeedLevel(int32 Delta) { SetNoteSpeedLevel(NoteSpeedLevel + Delta); }

	// 速度を初期値（プロジェクト設定の DefaultSpeedLevel、通常は 3）に戻す
	UFUNCTION(BlueprintCallable, Category="Rhythm Settings")
	void ResetNoteSpeedLevel();

	// 今の速度での到達時間（ノーツが出現してから判定地点に届くまでの秒数）
	UFUNCTION(BlueprintPure, Category="Rhythm Settings")
	float GetNoteArrivalTime() const;

	// 設定できる範囲（1 / 5）
	UFUNCTION(BlueprintPure, Category="Rhythm Settings")
	int32 GetMinNoteSpeedLevel() const;

	UFUNCTION(BlueprintPure, Category="Rhythm Settings")
	int32 GetMaxNoteSpeedLevel() const;

	// 速度が変わったとき（表示の更新などに使う）
	UPROPERTY(BlueprintAssignable, Category="Rhythm Settings")
	FOnNoteSpeedLevelChanged OnNoteSpeedLevelChanged;

	// ---- 難易度（NORMAL / HARD） ----

	UFUNCTION(BlueprintPure, Category="Rhythm Settings")
	ERhythmDifficulty GetDifficulty() const { return Difficulty; }

	// 難易度を設定する（速度は変えない）
	UFUNCTION(BlueprintCallable, Category="Rhythm Settings")
	void SetDifficulty(ERhythmDifficulty NewDifficulty);

	// 難易度が変わったとき
	UPROPERTY(BlueprintAssignable, Category="Rhythm Settings")
	FOnDifficultyChanged OnDifficultyChanged;

private:

	// セーブデータへの書き込み・読み込み
	void SaveSettings() const;
	void LoadSettings();

	int32 NoteSpeedLevel = 3;

	ERhythmDifficulty Difficulty = ERhythmDifficulty::Normal;
};
