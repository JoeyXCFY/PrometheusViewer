
#include "LineChartWidget.h"
#include "SlateBasics.h"
#include "SlateCore.h"
#include "Rendering/DrawElements.h"
#include "Math/UnrealMathUtility.h"

void ULineChartWidget::SetChartData(const TArray<FVector2D>& InDataPoints)
{
    DataPoints = InDataPoints;

    // 強制重新繪製
    if (IsInViewport())
    {
        Invalidate(EInvalidateWidget::LayoutAndVolatility);
    }
}

int32 ULineChartWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
        LayerId, InWidgetStyle, bParentEnabled);

    if (DataPoints.Num() < 2)
    {
        return LayerId;
    }

    // 畫布大小
    const FVector2D Size = AllottedGeometry.GetLocalSize();

    // 找出資料的 min/max
    float MinX = DataPoints[0].X;
    float MaxX = DataPoints[0].X;
    float MinY = DataPoints[0].Y;
    float MaxY = DataPoints[0].Y;

    for (const FVector2D& Point : DataPoints)
    {
        MinX = FMath::Min(MinX, Point.X);
        MaxX = FMath::Max(MaxX, Point.X);
        MinY = FMath::Min(MinY, Point.Y);
        MaxY = FMath::Max(MaxY, Point.Y);
    }

    // 避免除以0
    float RangeX = FMath::Max(MaxX - MinX, 1.0f);
    float RangeY = FMath::Max(MaxY - MinY, 1.0f);

    // 繪製資料線段
    for (int32 i = 0; i < DataPoints.Num() - 1; ++i)
    {
        FVector2D P0 = DataPoints[i];
        FVector2D P1 = DataPoints[i + 1];

        // 資料轉畫布座標（X從左到右，Y從下到上）
        FVector2D Start((P0.X - MinX) / RangeX * Size.X,
            Size.Y - (P0.Y - MinY) / RangeY * Size.Y);

        FVector2D End((P1.X - MinX) / RangeX * Size.X,
            Size.Y - (P1.Y - MinY) / RangeY * Size.Y);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            { Start, End },
            ESlateDrawEffect::None,
            FLinearColor::Green,
            true, // bAntiAlias
            2.0f  // 線寬
        );
    }

    return LayerId + 1;
}