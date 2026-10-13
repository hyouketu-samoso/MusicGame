#include "RhythmGameSettings.h"

URhythmGameSettings::URhythmGameSettings()
{
	// 難易度調整仕様書「ノーツ速度仕様」の仮設定値（速度1 → 5）
	SpeedArrivalTimes = { 2.4f, 2.1f, 1.8f, 1.5f, 1.2f };
}

float URhythmGameSettings::GetArrivalTime(int32 SpeedLevel) const
{
	const int32 Index = FMath::Clamp(SpeedLevel, MinSpeedLevel, MaxSpeedLevel) - MinSpeedLevel;

	// 0 秒以下は使わない（仕様書「到達時間は0秒より大きい値を使用」）
	return SpeedArrivalTimes.IsValidIndex(Index) ? FMath::Max(SpeedArrivalTimes[Index], 0.1f) : 1.8f;
}
