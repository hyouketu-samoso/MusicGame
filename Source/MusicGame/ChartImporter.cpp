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

	if (!FFileHelper::LoadFileToString(
		JsonString,
		*FilePath))
	{
		return false;
	}

	TSharedPtr<FJsonValue> RootValue;

	TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(
		Reader,
		RootValue
	) || !RootValue.IsValid())
	{
		return false;
	}

	if (RootValue->Type != EJson::Array)
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>& Events =
		RootValue->AsArray();

	OutNotes.Empty();

	TMap<int32, float> ActiveNotes;

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

		float Time =
			Obj->GetNumberField(TEXT("time"));

		int32 NoteNumber =
			Obj->GetIntegerField(TEXT("note"));

		FString EventType =
			Obj->GetStringField(TEXT("type"));

		if (EventType == TEXT("note_on"))
		{
			ActiveNotes.Add(
				NoteNumber,
				Time
			);
		}
		else if (EventType == TEXT("note_off"))
		{
			if (!ActiveNotes.Contains(NoteNumber))
			{
				continue;
			}

			float StartTime =
				ActiveNotes[NoteNumber];

			float Duration =
				Time - StartTime;

			FNoteData Data;

			Data.Time = StartTime;
			Data.Duration = Duration;
			Data.Lane = NoteNumber % 4;

			// Determine note type
			if (NoteNumber == 57)
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
				TEXT("=== NEW CHART IMPORTER === Note=%d Duration=%.3f Type=%s"),
				NoteNumber,
				Duration,
				*Data.Type
			);

			OutNotes.Add(Data);

			ActiveNotes.Remove(NoteNumber);
		}
	}

	return true;
}