
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

    const FVector2D Size = AllottedGeometry.GetLocalSize();

    // Padding 區域
    const float PaddingLeft = 50.0f;
    const float PaddingRight = 10.0f;
    const float PaddingTop = 10.0f;
    const float PaddingBottom = 30.0f;

    const FVector2D PlotOrigin = FVector2D(PaddingLeft, PaddingTop);
    const FVector2D PlotSize = FVector2D(Size.X - PaddingLeft - PaddingRight, Size.Y - PaddingTop - PaddingBottom);

    // 計算資料範圍
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

    float RangeX = FMath::Max(MaxX - MinX, 1.0f);
    float RangeY = FMath::Max(MaxY - MinY, 1.0f);

    // 畫 XY 軸
    FVector2D Origin = FVector2D(PaddingLeft, Size.Y - PaddingBottom);
    FVector2D XAxisEnd = FVector2D(Size.X - PaddingRight, Size.Y - PaddingBottom);
    FVector2D YAxisEnd = FVector2D(PaddingLeft, PaddingTop);

    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        { Origin, XAxisEnd }, ESlateDrawEffect::None, FLinearColor::White, true, 1.0f);
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        { Origin, YAxisEnd }, ESlateDrawEffect::None, FLinearColor::White, true, 1.0f);

    // 字體設定
    FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle("NormalFont");
    FontInfo.Size = 10;

    // 畫 X 軸刻度 (時間格式)
    const int32 NumXTicks = 5;
    for (int32 i = 0; i <= NumXTicks; ++i)
    {
        float Alpha = (float)i / NumXTicks;
        float Value = FMath::Lerp(MinX, MaxX, Alpha);
        float X = PlotOrigin.X + Alpha * PlotSize.X;

        // 時間格式（Unix timestamp → HH:MM:SS）
        FDateTime DateTime = FDateTime::FromUnixTimestamp((int64)Value);
        FString Label = DateTime.ToString(TEXT("%H:%M:%S"));

        // Tick
        FVector2D Start(X, Origin.Y);
        FVector2D End(X, Origin.Y + 5.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            { Start, End }, ESlateDrawEffect::None, FLinearColor::White, true, 1.0f);

        // Label
        FVector2D TextPos = FVector2D(X - 20, Origin.Y + 8);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(TextPos, FVector2D(40, 12)),
            FText::FromString(Label), FontInfo, ESlateDrawEffect::None, FLinearColor::White);
    }

    // 畫 Y 軸刻度
    const int32 NumYTicks = 5;
    for (int32 i = 0; i <= NumYTicks; ++i)
    {
        float Alpha = (float)i / NumYTicks;
        float Y = PlotOrigin.Y + (1 - Alpha) * PlotSize.Y;
        float Value = FMath::Lerp(MinY, MaxY, Alpha);

        // Tick
        FVector2D Start(Origin.X - 5, Y);
        FVector2D End(Origin.X, Y);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            { Start, End }, ESlateDrawEffect::None, FLinearColor::White, true, 1.0f);

        // Label
        FString Label = FString::Printf(TEXT("%.1f"), Value);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(0, Y - 6), FVector2D(40, 12)),
            FText::FromString(Label), FontInfo, ESlateDrawEffect::None, FLinearColor::White);
    }

    LayerId++;

    // 畫資料折線
    for (int32 i = 0; i < DataPoints.Num() - 1; ++i)
    {
        const FVector2D P0 = FVector2D(DataPoints[i].X, DataPoints[i].Y);
        const FVector2D P1 = FVector2D(DataPoints[i + 1].X, DataPoints[i + 1].Y);

        FVector2D Start;
        Start.X = PlotOrigin.X + ((P0.X - MinX) / RangeX) * PlotSize.X;
        Start.Y = PlotOrigin.Y + (1.0f - (P0.Y - MinY) / RangeY) * PlotSize.Y;

        FVector2D End;
        End.X = PlotOrigin.X + ((P1.X - MinX) / RangeX) * PlotSize.X;
        End.Y = PlotOrigin.Y + (1.0f - (P1.Y - MinY) / RangeY) * PlotSize.Y;

        // 保證不超出繪圖邊界
        Start.X = FMath::Clamp(Start.X, PlotOrigin.X, PlotOrigin.X + PlotSize.X);
        Start.Y = FMath::Clamp(Start.Y, PlotOrigin.Y, PlotOrigin.Y + PlotSize.Y);
        End.X = FMath::Clamp(End.X, PlotOrigin.X, PlotOrigin.X + PlotSize.X);
        End.Y = FMath::Clamp(End.Y, PlotOrigin.Y, PlotOrigin.Y + PlotSize.Y);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            { Start, End },
            ESlateDrawEffect::None,
            FLinearColor::Green,
            true,
            2.0f
        );
    }


    return LayerId + 1;
}