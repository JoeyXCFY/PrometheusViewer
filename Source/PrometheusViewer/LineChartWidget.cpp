
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

    // Padding
    const float PaddingLeft = 50.0f;
    const float PaddingRight = 10.0f;
    const float PaddingTop = 10.0f;
    const float PaddingBottom = 30.0f;

    const FVector2D PlotOrigin(PaddingLeft, PaddingTop);
    const FVector2D PlotSize(Size.X - PaddingLeft - PaddingRight, Size.Y - PaddingTop - PaddingBottom);

    // Step 1: 將 X 軸轉換為相對時間（避免 float 精度問題）
    TArray<FVector2D> RelativeDataPoints = DataPoints;
    float BaseTime = DataPoints[0].X;
    for (FVector2D& Point : RelativeDataPoints)
    {
        Point.X -= BaseTime;
    }

    // Step 2: 計算資料範圍
    float MinX = RelativeDataPoints[0].X;
    float MaxX = RelativeDataPoints[0].X;
    float MinY = RelativeDataPoints[0].Y;
    float MaxY = RelativeDataPoints[0].Y;

    for (const FVector2D& Point : RelativeDataPoints)
    {
        MinX = FMath::Min(MinX, Point.X);
        MaxX = FMath::Max(MaxX, Point.X);
        MinY = FMath::Min(MinY, Point.Y);
        MaxY = FMath::Max(MaxY, Point.Y);
    }

    float RangeX = FMath::Max(MaxX - MinX, 1.0f);
    float RangeY = FMath::Max(MaxY - MinY, 1.0f);

    if (RangeX < 1.0f)
    {
        RangeX = 1.0f;
        MinX -= 0.5f;
        MaxX += 0.5f;
    }

    if (RangeY < 1.0f)
    {
        RangeY = 1.0f;
        MinY -= 0.5f;
        MaxY += 0.5f;
    }

    // 畫 XY 軸
    FVector2D Origin(PaddingLeft, Size.Y - PaddingBottom);
    FVector2D XAxisEnd(Size.X - PaddingRight, Size.Y - PaddingBottom);
    FVector2D YAxisEnd(PaddingLeft, PaddingTop);

    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        { Origin, XAxisEnd }, ESlateDrawEffect::None, FLinearColor::White, true, 1.0f);
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        { Origin, YAxisEnd }, ESlateDrawEffect::None, FLinearColor::White, true, 1.0f);

    FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle("NormalFont");
    FontInfo.Size = 10;

    // 畫 X 軸刻度
    const int32 NumXTicks = 5;
    TSet<FString> SeenLabels;

    for (int32 i = 0; i <= NumXTicks; ++i)
    {
        float Alpha = static_cast<float>(i) / NumXTicks;
        float RelativeValue = FMath::Lerp(MinX, MaxX, Alpha);
        float X = PlotOrigin.X + Alpha * PlotSize.X;

        // 反推回原始時間
        int64 OriginalUnix = static_cast<int64>(FMath::RoundHalfToZero(RelativeValue + BaseTime));
        FDateTime UtcDateTime = FDateTime::FromUnixTimestamp(OriginalUnix);
        FTimespan TimeZoneOffset = FDateTime::Now() - FDateTime::UtcNow();
        FDateTime LocalDateTime = UtcDateTime + TimeZoneOffset;
        FString Label = LocalDateTime.ToString(TEXT("%H:%M:%S"));

        if (SeenLabels.Contains(Label))
            continue;
        SeenLabels.Add(Label);

        FVector2D Start(X, Origin.Y);
        FVector2D End(X, Origin.Y + 5.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            { Start, End }, ESlateDrawEffect::None, FLinearColor::White, true, 1.0f);

        FVector2D TextPos(X - 20, Origin.Y + 8);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(TextPos, FVector2D(40, 12)),
            FText::FromString(Label), FontInfo, ESlateDrawEffect::None, FLinearColor::White);
    }

    // 畫 Y 軸刻度
    const int32 NumYTicks = 5;
    for (int32 i = 0; i <= NumYTicks; ++i)
    {
        float Alpha = static_cast<float>(i) / NumYTicks;
        float Y = PlotOrigin.Y + (1 - Alpha) * PlotSize.Y;
        float Value = FMath::Lerp(MinY, MaxY, Alpha);

        FVector2D Start(Origin.X - 5, Y);
        FVector2D End(Origin.X, Y);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            { Start, End }, ESlateDrawEffect::None, FLinearColor::White, true, 1.0f);

        FString Label = FString::Printf(TEXT("%.1f"), Value);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(0, Y - 6), FVector2D(40, 12)),
            FText::FromString(Label), FontInfo, ESlateDrawEffect::None, FLinearColor::White);
    }

    LayerId++;

    // 畫折線
    for (int32 i = 0; i < RelativeDataPoints.Num() - 1; ++i)
    {
        const FVector2D& P0 = RelativeDataPoints[i];
        const FVector2D& P1 = RelativeDataPoints[i + 1];

        // 檢查是否重複點（避免垂直線）
        if (FMath::IsNearlyEqual(P0.X, P1.X, KINDA_SMALL_NUMBER))
        {
            continue;
        }

        FVector2D Start;
        Start.X = PlotOrigin.X + ((P0.X - MinX) / RangeX) * PlotSize.X;
        Start.Y = PlotOrigin.Y + (1.0f - (P0.Y - MinY) / RangeY) * PlotSize.Y;

        FVector2D End;
        End.X = PlotOrigin.X + ((P1.X - MinX) / RangeX) * PlotSize.X;
        End.Y = PlotOrigin.Y + (1.0f - (P1.Y - MinY) / RangeY) * PlotSize.Y;

        Start.X = FMath::Clamp(Start.X, PlotOrigin.X, PlotOrigin.X + PlotSize.X);
        Start.Y = FMath::Clamp(Start.Y, PlotOrigin.Y, PlotOrigin.Y + PlotSize.Y);
        End.X = FMath::Clamp(End.X, PlotOrigin.X, PlotOrigin.X + PlotSize.X);
        End.Y = FMath::Clamp(End.Y, PlotOrigin.Y, PlotOrigin.Y + PlotSize.Y);

        FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(), { Start, End },
            ESlateDrawEffect::None, FLinearColor::Green, true, 2.0f);
    }

    return LayerId + 1;
}