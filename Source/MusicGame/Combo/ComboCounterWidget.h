// ============================================================
// ComboCounterWidget
//  役割 : コンボの数字を、画像(0?9)で表示する部品
//  動き : 数字が変わるとき、古い数字が上に流れて消え、新しい数字が下から出てくる
//         (桁ごとに動く。変わった桁だけが動く)
//  使い方 : SetCombo(コンボ数) を呼ぶだけ。0 以下のときは、自分で隠れる
//  注意 : UWidget に「Slot」というメンバーがあるので、変数名に Slot は使わない
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ComboCounterWidget.generated.h"

class UHorizontalBox;
class UImage;
class USizeBox;
class UTexture2D;

// 1桁ぶんの部品(画像2枚 = 古い数字と、新しい数字)
USTRUCT()
struct FComboDigitSlot
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<USizeBox> Box;        // この桁の枠(はみ出た部分を切り取る)
	UPROPERTY() TObjectPtr<UImage>   OldImage;   // 古い数字(上に流れて消える)
	UPROPERTY() TObjectPtr<UImage>   NewImage;   // 新しい数字(下から出てくる)

	int32 Digit = -1;          // いま表示している数字(-1 は、まだ何もない)
	float Elapsed = 0.0f;      // 動き始めてからの秒数
	bool  bAnimating = false;  // 動いている最中か
};

UCLASS()
class MUSICGAME_API UComboCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// コンボ数を渡す(変わった桁だけが、動く)。0 以下のときは、隠す
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void SetCombo(int32 Combo);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ---- 調整値(WBP のクラスデフォルトで変える) ----

	// 0?9 の画像(10個。0 番目が「0」、9 番目が「9」)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TArray<TObjectPtr<UTexture2D>> DigitTextures;

	// 1桁の大きさ(画像の比率 2:3 に合わせる)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FVector2D DigitSize = FVector2D(64.0f, 96.0f);

	// 数字が切り替わる秒数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	float RollDuration = 0.25f;

	// 流れる距離(桁の高さに対する割合。0.6 = 高さの 60%)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	float RollDistanceRatio = 0.6f;

	// ---- WBP側に、同じ名前の部品を置く(「変数である」にチェック) ----
	// 桁を横に並べる箱(WBP_ComboCounter の一番上(ルート)に置く Horizontal Box)
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UHorizontalBox> DigitContainer;

private:
	// 桁の部品を1つ作る(左端に足す)
	void AddSlotAtLeft();

	// 桁に、数字の画像をセットして、動きを始める(bAnimate が false のときは、すぐ切り替える)
	void ShowDigit(FComboDigitSlot& DigitSlot, int32 NewDigit, bool bAnimate);

	// 数字(0?9)の画像を返す(無いときは nullptr)
	UTexture2D* GetDigitTexture(int32 Digit) const;

	UPROPERTY()
	TArray<FComboDigitSlot> DigitSlots;   // 左から右の順(最後が1の位)
};