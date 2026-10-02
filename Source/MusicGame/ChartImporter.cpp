// ChartImporter.cpp
#include "ChartImporter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool UChartImporter::LoadChart(const FString& FilePath, TArray<FNoteData>& OutNotes)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
        return false;

    TSharedPtr<FJsonValue> RootValue;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
        return false;

    const TArray<TSharedPtr<FJsonValue>> Events = RootValue->AsArray();

    OutNotes.Empty();
    TMap<int32, float> ActiveNotes; // note_on の開始時間を記録

    for (auto& EventValue : Events)
    {
        TSharedPtr<FJsonObject> Obj = EventValue->AsObject();
        if (!Obj.IsValid()) continue;

        float Time = Obj->GetNumberField("time");
        int32 NoteNumber = Obj->GetIntegerField("note");
        FString EventType = Obj->GetStringField("type");

        if (EventType == "note_on")
        {
            ActiveNotes.Add(NoteNumber, Time);
        }
        else if (EventType == "note_off")
        {
            if (!ActiveNotes.Contains(NoteNumber)) continue;

            float StartTime = ActiveNotes[NoteNumber];
            float Duration = Time - StartTime;

            FNoteData Data;
            Data.Time = StartTime;
            Data.Duration = Duration;
            Data.Lane = NoteNumber % 4;   // とりあえずレーンは note % 4
            Data.Type = TEXT("tap");      // まず全部 tap でOK

            OutNotes.Add(Data);
            ActiveNotes.Remove(NoteNumber);
        }
    }

    return true;
}
