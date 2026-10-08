//#include "ChartImporter.h"
//
//#include "Misc/FileHelper.h"
//#include "Misc/Paths.h"
//#include "Dom/JsonObject.h"
//#include "Serialization/JsonReader.h"
//#include "Serialization/JsonSerializer.h"
//
//bool UChartImporter::LoadChart(
//	const FString& FilePath,
//	TArray<FNoteData>& OutNotes
//)
//{
//	FString JsonString;
//
//	if (!FFileHelper::LoadFileToString(
//		JsonString,
//		*FilePath))
//	{
//		return false;
//	}
//
//	TSharedPtr<FJsonValue> RootValue;
//
//	TSharedRef<TJsonReader<>> Reader =
//		TJsonReaderFactory<>::Create(JsonString);
//
//	if (!FJsonSerializer::Deserialize(
//		Reader,
//		RootValue
//	) || !RootValue.IsValid())
//	{
//		return false;
//	}
//
//	if (RootValue->Type != EJson::Array)
//	{
//		return false;
//	}
//
//	const TArray<TSharedPtr<FJsonValue>>& Events =
//		RootValue->AsArray();
//
//	OutNotes.Empty();
//
//	TMap<int32, float> ActiveNotes;
//
//	for (const TSharedPtr<FJsonValue>& EventValue : Events)
//	{
//		if (!EventValue.IsValid())
//		{
//			continue;
//		}
//
//		TSharedPtr<FJsonObject> Obj =
//			EventValue->AsObject();
//
//		if (!Obj.IsValid())
//		{
//			continue;
//		}
//
//		float Time =
//			Obj->GetNumberField(TEXT("time"));
//
//		int32 NoteNumber =
//			Obj->GetIntegerField(TEXT("note"));
//
//		FString EventType =
//			Obj->GetStringField(TEXT("type"));
//
//		if (EventType == TEXT("note_on"))
//		{
//			ActiveNotes.Add(
//				NoteNumber,
//				Time
//			);
//		}
//		else if (EventType == TEXT("note_off"))
//		{
//			if (!ActiveNotes.Contains(NoteNumber))
//			{
//				continue;
//			}
//
//			float StartTime =
//				ActiveNotes[NoteNumber];
//
//			float Duration =
//				Time - StartTime;
//
//			FNoteData Data;
//
//			Data.Time = StartTime;
//			Data.Duration = Duration;
//			Data.Lane = NoteNumber % 4;
//
//			// Determine note type
//			if (NoteNumber == 57)
//			{
//				Data.Type = TEXT("flick");
//			}
//			else if (Duration >= 1.0f)
//			{
//				Data.Type = TEXT("hold");
//			}
//			else
//			{
//				Data.Type = TEXT("tap");
//			}
//
//			UE_LOG(
//				LogTemp,
//				Warning,
//				TEXT("=== NEW CHART IMPORTER === Note=%d Duration=%.3f Type=%s"),
//				NoteNumber,
//				Duration,
//				*Data.Type
//			);
//
//			OutNotes.Add(Data);
//
//			ActiveNotes.Remove(NoteNumber);
//		}
//	}
//
//	return true;
//}

#include "ChartImporter.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool UChartImporter::LoadChart(
	const FString& FilePath,
	TArray<FNoteData>& OutNotes
)
{
	FString JsonString;

	// ============================================================
	// JSONファイル読み込み
	// ============================================================

	if (!FFileHelper::LoadFileToString(
		JsonString,
		*FilePath))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ChartImporter: JSONファイルを開けません: %s"),
			*FilePath
		);

		return false;
	}

	// ============================================================
	// JSON解析
	// ============================================================

	TSharedPtr<FJsonValue> RootValue;

	TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(
		Reader,
		RootValue
	) || !RootValue.IsValid())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ChartImporter: JSON parse failed")
		);

		return false;
	}

	// ============================================================
	// ルートが配列か確認
	// ============================================================

	if (RootValue->Type != EJson::Array)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ChartImporter: Root is not an array")
		);

		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>& Events =
		RootValue->AsArray();

	OutNotes.Empty();

	// ============================================================
	// 同じMIDIノート番号が複数存在するため、
	// 1個ではなく配列で管理
	// ============================================================

	struct FActiveNote
	{
		float StartTime = 0.0f;
		int32 Lane = 0;
		int32 NoteNumber = 0;
		int32 OutNoteIndex = INDEX_NONE;
	};

	TMap<int32, TArray<FActiveNote>> ActiveNotes;

	// ============================================================
	// JSONイベント処理
	// ============================================================

	for (const TSharedPtr<FJsonValue>& EventValue : Events)
	{
		if (!EventValue.IsValid())
		{
			continue;
		}

		TSharedPtr<FJsonObject> Obj =
			EventValue->AsObject();

		if (!Obj.IsValid())
		{
			continue;
		}

		if (!Obj->HasField(TEXT("time")) ||
			!Obj->HasField(TEXT("type")))
		{
			continue;
		}

		const float Time =
			Obj->GetNumberField(TEXT("time"));

		const FString EventType =
			Obj->GetStringField(TEXT("type"));

		const int32 NoteNumber =
			Obj->HasField(TEXT("note"))
			? Obj->GetIntegerField(TEXT("note"))
			: 0;

		// ========================================================
		// NOTE ON
		// ========================================================

		if (EventType == TEXT("note_on"))
		{
			int32 Lane = 0;

			if (Obj->HasField(TEXT("lane")))
			{
				Lane =
					Obj->GetIntegerField(TEXT("lane"));
			}
			else
			{
				// laneが無い場合の予備
				Lane = NoteNumber % 4;
			}

			// 4レーンに制限
			Lane = FMath::Clamp(Lane, 0, 3);

			FNoteData Data;

			Data.Time = Time;
			Data.Duration = 0.0f;
			Data.Lane = Lane;

			// MIDI 57はflick
			if (NoteNumber == 57)
			{
				Data.Type = TEXT("flick");
			}
			else
			{
				Data.Type = TEXT("tap");
			}

			// OutNotesへ追加
			const int32 OutNoteIndex =
				OutNotes.Add(Data);

			// NOTE OFFとの対応用
			FActiveNote Active;

			Active.StartTime = Time;
			Active.Lane = Lane;
			Active.NoteNumber = NoteNumber;
			Active.OutNoteIndex = OutNoteIndex;

			ActiveNotes
				.FindOrAdd(NoteNumber)
				.Add(Active);

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[CHART ON] Time=%.3f Note=%d Lane=%d"),
				Time,
				NoteNumber,
				Lane
			);
		}

		// ========================================================
		// NOTE OFF
		// ========================================================

		else if (EventType == TEXT("note_off"))
		{
			TArray<FActiveNote>* Queue =
				ActiveNotes.Find(NoteNumber);

			if (Queue == nullptr ||
				Queue->Num() == 0)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[CHART OFF] 対応するNOTE ONがありません Note=%d Time=%.3f"),
					NoteNumber,
					Time
				);

				continue;
			}

			// 先に登録されたNOTE ONから対応
			const FActiveNote Active =
				(*Queue)[0];

			Queue->RemoveAt(0);

			if (Queue->Num() == 0)
			{
				ActiveNotes.Remove(NoteNumber);
			}

			if (!OutNotes.IsValidIndex(
				Active.OutNoteIndex))
			{
				continue;
			}

			FNoteData& Data =
				OutNotes[Active.OutNoteIndex];

			const float Duration =
				FMath::Max(
					Time - Active.StartTime,
					0.0f
				);

			Data.Duration = Duration;

			// ====================================================
			// ノーツ種類判定
			// ====================================================

			if (Active.NoteNumber == 57)
			{
				Data.Type = TEXT("flick");
			}
			else if (Duration >= 1.0f)
			{
				Data.Type = TEXT("hold");
			}
			else
			{
				Data.Type = TEXT("tap");
			}

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[CHART OFF] Time=%.3f Note=%d Lane=%d Duration=%.3f Type=%s"),
				Time,
				Active.NoteNumber,
				Data.Lane,
				Duration,
				*Data.Type
			);
		}
	}

	// ============================================================
	// 時間順にソート
	// ============================================================

	OutNotes.Sort(
		[](const FNoteData& A, const FNoteData& B)
		{
			return A.Time < B.Time;
		}
	);

	// ============================================================
	// 完了ログ
	// ============================================================

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("ChartImporter: Events=%d Notes=%d"),
		Events.Num(),
		OutNotes.Num()
	);

	return true;
}