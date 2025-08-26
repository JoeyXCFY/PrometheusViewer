#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LineChartWidget.generated.h"

UCLASS()
class PROMETHEUSVIEWER_API ULineChartWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 設定圖表資料
    UFUNCTION(BlueprintCallable, Category = "Chart")
    void SetChartData(const TArray<FVector2D>& InDataPoints);

    UFUNCTION(BlueprintCallable, Category = "LineChart")
    void AddDataPoint(float X, float Y);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart")
	int32 UserTimezone;

    FVector2D CachedMousePosition;
    bool bMouseHovered = false;
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

    FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;


private:
    UPROPERTY()
    TArray<FVector2D> DataPoints;

    int32 MaxPoints = 300;
};