// ============================================================
// ScoreCounterWidget
//  役割 : スコアを、画像の数字(0?9)で表示して、増えたときに動きをつける部品
//  動き : ・変わった桁だけ、少し下がって薄くなり、入れ替わって、上から戻ってくる
//         ・「+100」のような加算分 : 数字の裏から右に滑り出て、しばらくしたら消える
//  使い方 : SetScore(今のスコア) を呼ぶだけ(増えた分は、自分で計算する)
//  画像 : コンボと同じ T_ComboNum_0?9 と、「+」の画像 T_ComboNum_Plus を使う
//  注意 : UWidget に「Slot」というメンバーがあるので、変数名に Slot は使わない
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreCounterWidget.generated.h"

class UHorizontalBox;
class UImage;
class USizeBox;
class UTexture2D;

// 1桁ぶんの部品(画像2枚 = 古い数字と、新しい数字)
USTRUCT()
struct FScoreDigitSlot
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UImage> OldImage;   // 古い数字(下がって消える)
	UPROPERTY() TObjectPtr<UImage> NewImage;   // 新しい数字(上から戻ってくる)

	int32 Digit = -1;           // いま表示している数字(-1 は、まだ何もない)
	float Elapsed = 0.0f;       // 動き始めてからの秒数
	bool  bAnimating = false;   // 動いている最中か
	bool  bHasOld = false;      // 古い数字の画像が入っているか
};

UCLASS()
class MUSICGAME_API UScoreCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 今のスコアを渡す
	//  ・増えたとき : 変わった桁が動いて、増えた分(+100 など)が出る
	//  ・減ったとき : 動かさずに、すぐ切り替える(増えた分は出さない)
	UFUNCTION(BlueprintCallable, Category = "Score")
	void SetScore(int32 Score);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ---- 画像(WBP のクラスデフォルトで入れる) ----

	// 0?9 の画像(10個。0 番目が「0」、9 番目が「9」)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	TArray<TObjectPtr<UTexture2D>> DigitTextures;

	// 「+」の画像(加算分の表示に使う。空のときは、数字だけを出す)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	TObjectPtr<UTexture2D> PlusTexture;

	// ---- 調整値 ----

	// スコアの桁数(5 なら「00123」。これより大きい値は、99999 で止まる)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	int32 NumDigits = 5;

	// スコア1桁の大きさ(画像の比率 2:3 に合わせる)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	FVector2D DigitSize = FVector2D(40.0f, 60.0f);

	// 加算分(+100)の1文字の大きさ(スコアより小さめ)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score|Gain")
	FVector2D GainDigitSize = FVector2D(24.0f, 36.0f);

	// 数字が切り替わる秒数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	float RollDuration = 0.25f;

	// 数字が上下に動く距離(ピクセル)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score")
	float RollDistance = 12.0f;

	// 加算分(+100)が滑り出る秒数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score|Gain")
	float GainSlideDuration = 0.2f;

	// 加算分が、数字の裏から滑り出る距離(ピクセル)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score|Gain")
	float GainSlideDistance = 40.0f;

	// 加算分を出しておく秒数(0 以下にすると、次に変わるまで出しっぱなし)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score|Gain")
	float GainHoldSeconds = 1.5f;

	// 加算分が消える秒数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Score|Gain")
	float GainFadeOutSeconds = 0.3f;

	// ---- WBP側に、同じ名前の部品を置く(「変数である」にチェック) ----
	// スコアの数字を並べる箱(Horizontal Box。必須。中身は C++ が作る)
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UHorizontalBox> ScoreContainer;
	// 加算分(+100)を並べる箱(Horizontal Box。無くても動く。中身は C++ が作る)
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> GainContainer;

private:
	// 桁の部品を NumDigits 個、左から順に作る
	void BuildScoreSlots();

	// スコアを、桁ごとに、画像にセットする(bAnimate が true のときは、変わった桁だけ動かす)
	void ApplyScore(int32 Score, bool bAnimate);

	// 桁に、数字の画像をセットして、動きを始める
	void ShowDigit(FScoreDigitSlot& DigitSlot, int32 NewDigit, bool bAnimate);

	// 加算分の画像(+ と数字)を作り直す
	void BuildGainImages(int32 Delta);

	// 加算分を消す
	void HideGain();

	// 数字(0?9)の画像を返す(無いときは nullptr)
	UTexture2D* GetDigitTexture(int32 Digit) const;

	// 大きさを決めた枠の中に、画像を1枚入れた部品を作る(加算分用)
	USizeBox* MakeFixedImage(UTexture2D* Texture, const FVector2D& Size);

	UPROPERTY()
	TArray<FScoreDigitSlot> DigitSlots;   // 左から右の順(最後が1の位)

	int32 CurrentScore = 0;       // 最後に受け取ったスコア
	bool  bInitialized = false;   // 最初の SetScore のときは、動かさない

	// 加算分の動き
	bool  bGainActive = false;
	float GainElapsed = 0.0f;
};