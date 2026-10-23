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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputOffsetChanged, int32, NewOffsetMs);

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

	// 入力タイミング調整値（ミリ秒）
	UPROPERTY()
	int32 InputOffsetMs = 0;
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

	// ---- 入力タイミング調整(ミリ秒) ----
	// 判定は「入力時刻 - 調整値」で行う。遅れて押す人はプラス、早く押す人はマイナス
	static constexpr int32 MaxInputOffsetMs = 160;

	UFUNCTION(BlueprintPure, Category = "Rhyyhm Settings")
	int32 GetInputOffsetMs() const { return InputOffsetMs; }

	UFUNCTION(BlueprintPure, Category = "Rhythm Settings")
	float GetInputOffsetSec() const { return InputOffsetMs / 1000.0f; }

	UFUNCTION(BlueprintCallable, Category = "Rhythm Settings")
	void SetInputOffsetMs(int32 NewOffsetMs);

	UFUNCTION(BlueprintCallable, Category = "Rhythm Settings")
	void ResetInputOffset() { SetInputOffsetMs(0); }

	UPROPERTY(BlueprintAssignable, Category = "Rhythm Settings")
	FOnInputOffsetChanged OnInputOffsetChanged;

	// デッドゾーン
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	float GetDeadZone() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void SetDeadZone(float NewDeadZone);

	// 感度
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	int32 GetSensitivityLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void SetSensitivityLevel(int32 NewLevel);

	// 振動
	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	bool GetVibrationEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void SetVibrationEnabled(bool bEnabled);

	// マスター音量
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	int32 GetMasterVolumeLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolumeLevel(int32 NewLevel);

	// BGM音量
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	int32 GetBGMVolumeLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetBGMVolumeLevel(int32 NewLevel);

	// SE音量
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	int32 GetSEVolumeLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSEVolumeLevel(int32 NewLevel);

	// 背景の明るさ
	UFUNCTION(BlueprintCallable, Category = "Settings|Brightness")
	int32 GetBGBrightnessStep() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Brightness")
	void SetBGBrightnessStep(int32 NewStep);

	// レーンの明るさ
	UFUNCTION(BlueprintCallable, Category = "Settings|Brightness")
	int32 GetLaneBrightnessStep() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Brightness")
	void SetLaneBrightnessStep(int32 NewStep);

	// ノーツの明るさ
	UFUNCTION(BlueprintCallable, Category = "Settings|Brightness")
	int32 GetNoteBrightnessStep() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Brightness")
	void SetNoteBrightnessStep(int32 NewStep);
private:

	// セーブデータへの書き込み・読み込み
	void SaveSettings() const;
	void LoadSettings();

	int32 NoteSpeedLevel = 3;

	ERhythmDifficulty Difficulty = ERhythmDifficulty::Normal;

	int32 InputOffsetMs = 0;

	// 入力設定
	UPROPERTY()
	float DeadZone = 0.20f;

	UPROPERTY()
	int32 SensitivityLevel = 3;

	UPROPERTY()
	bool bVibration = true;

	// 音量設定
	UPROPERTY()
	int32 MasterVolumeLevel = 100;

	UPROPERTY()
	int32 BGMVolumeLevel = 100;

	UPROPERTY()
	int32 SEVolumeLevel = 100;

	// 明るさ設定（0～10）
	UPROPERTY()
	int32 BGBrightnessStep = 5;

	UPROPERTY()
	int32 LaneBrightnessStep = 5;

	UPROPERTY()
	int32 NoteBrightnessStep = 5;
};
