// ============================================================
// GameSessionSubsystem
//  役割 : レベルを開き直しても消えない「ゲーム全体の記憶」
//         リトライで開き直したときに、曲選択を飛ばすための印を持つ
//  使い方 : 曲選択を出す場所で ConsumeSkipSongSelect() を呼ぶ(下のコメント参照)
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameSessionSubsystem.generated.h"

UCLASS()
class MUSICGAME_API UGameSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// リザルトのリトライが呼ぶ。「次のレベル開始では曲選択を出さない」という印をつける
	UFUNCTION(BlueprintCallable, Category = "Rhythm")
	void RequestSkipSongSelect() { bSkipSongSelect = true; }

	// 曲選択を出す場所が呼ぶ。true なら曲選択を出さず、前回の曲でそのまま始める
	// 印は読むと消える(その次に開いたときは、ふつうに曲選択が出る)
	UFUNCTION(BlueprintCallable, Category = "Rhythm")
	bool ConsumeSkipSongSelect()
	{
		const bool bResult = bSkipSongSelect;
		bSkipSongSelect = false;
		return bResult;
	}

	// 曲を開始するとき、PlayerController が呼ぶ。選んだ曲の番号を覚えておく
	void SetLastSongIndex(int32 Index) { LastSongIndex = Index; }

	// リトライで開き直したとき、前回の曲番号を取り出す(未設定なら INDEX_NONE)
	int32 GetLastSongIndex() const { return LastSongIndex; }

private:
	bool bSkipSongSelect = false;
	int32 LastSongIndex = INDEX_NONE;
};