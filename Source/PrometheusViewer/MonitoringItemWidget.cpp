// Fill out your copyright notice in the Description page of Project Settings.


#include "MonitoringItemWidget.h"
#include "PrometheusManager.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "LineChartWidget.h"

void UMonitoringItemWidget::InitializeOptions(APrometheusManager* Manager)
{
    this->ManagerRef = Manager;
    if (!Manager) return;

    if (!Manager->OnMetricsFetched.IsAlreadyBound(this, &UMonitoringItemWidget::OnMetricsReady))
    {
        Manager->OnMetricsFetched.AddDynamic(this, &UMonitoringItemWidget::OnMetricsReady);
    }

    if (!MetricComboBox->OnSelectionChanged.IsBound())
    {
        MetricComboBox->OnSelectionChanged.AddDynamic(this, &UMonitoringItemWidget::OnMetricChanged);
    }
    if (!TypeComboBox->OnSelectionChanged.IsBound())
    {
        TypeComboBox->OnSelectionChanged.AddDynamic(this, &UMonitoringItemWidget::OnTypeChanged);
    }

    if (TypeComboBox->GetOptionCount() == 0)
    {
        TypeComboBox->AddOption("Raw");
        TypeComboBox->AddOption("Usage%");
    }

    if (!Manager->OnRangeQueryResponse.IsAlreadyBound(this, &UMonitoringItemWidget::OnRangeQueryResponseReceived))
    {
        Manager->OnRangeQueryResponse.AddDynamic(this, &UMonitoringItemWidget::OnRangeQueryResponseReceived);
    }

    Manager->FetchAvailableMetrics();
}

void UMonitoringItemWidget::OnMetricsReady(const TArray<FString>& Metrics)
{
    if (!bMetricsInitialized)
    {
        MetricComboBox->ClearOptions();
        for (const FString& M : Metrics)
        {
            MetricComboBox->AddOption(M);
        }
        bMetricsInitialized = true;
    }
}

void UMonitoringItemWidget::OnMetricChanged(FString Selected, ESelectInfo::Type)
{
    SelectedMetric = Selected;
    UE_LOG(LogTemp, Warning, TEXT("[ComboBox] Metric changed: %s"), *Selected);

    if (!SelectedType.IsEmpty())
    {
        const FString FinalPromQL = GeneratePromQL(SelectedMetric, SelectedType);

        if (ManagerRef)
        {
            // 送範圍查詢 (例如最近 300 秒，每 15 秒取一點)
            ManagerRef->HandleRangeQuery(FinalPromQL, 300.f, 5.f);
        }

        OnPromQueryGenerated.Broadcast(FinalPromQL, this);
    }
}

void UMonitoringItemWidget::OnTypeChanged(FString Selected, ESelectInfo::Type)
{
    SelectedType = Selected;

    if (!SelectedMetric.IsEmpty())
    {
        const FString FinalPromQL = GeneratePromQL(SelectedMetric, SelectedType);
        if (ManagerRef)
        {
            ManagerRef->HandleRangeQuery(FinalPromQL, 300.f, 5.f);
        }
        OnPromQueryGenerated.Broadcast(FinalPromQL, this);
    }
}

FString UMonitoringItemWidget::GeneratePromQL(const FString& Metric, const FString& Type) const
{
    FString Result = ManagerRef->GetPromQLFromMapping(Metric, Type);
    UE_LOG(LogTemp, Warning, TEXT("[PromQL Generate] Metric: %s, Type: %s, Result: %s"), *Metric, *Type, *Result);
    if (!ManagerRef) {
        
        return "";
    }

    return ManagerRef->GetPromQLFromMapping(Metric, Type);
}

void UMonitoringItemWidget::TriggerQuery(APrometheusManager* Manager)
{
    if (!Manager) return;

    LastSentPromQL = GeneratePromQL(SelectedMetric, SelectedType);

    Manager->OnQueryResponse.AddDynamic(this, &UMonitoringItemWidget::OnQueryResponseReceived);
    Manager->HandleQuery(LastSentPromQL);
    Manager->RegisterQuery(LastSentPromQL);
}

void UMonitoringItemWidget::OnQueryResponseReceived(const FString& PromQL, const FString& Result)
{
    UE_LOG(LogTemp, Warning, TEXT("Widget OnQueryResponseReceived - MyLast: %s | Incoming: %s"), *LastSentPromQL, *PromQL);
    UE_LOG(LogTemp, Warning, TEXT("[QueryResult] PromQL: %s => Result: %s"), *PromQL, *Result);

    if (!ResultText)
    {
        UE_LOG(LogTemp, Error, TEXT("ResultText is nullptr!"));
    }

    if (PromQL == LastSentPromQL) {

        float FloatValue = FCString::Atof(*Result);
        if (ResultText)
        {
            FString FormattedResult = FString::Printf(TEXT("%.3f"), FloatValue);
            ResultText->SetText(FText::FromString(FormattedResult));
        }

    }

}

void UMonitoringItemWidget::OnRangeQueryResponseReceived(const FString& PromQL, const TArray<FVector2D>& DataPoints)
{
    if (PromQL == LastSentPromQL && LineChartResult)
    {
        InitializeChartWithHistory(DataPoints);
    }
}


void UMonitoringItemWidget::InitializeChartWithHistory(const TArray<FVector2D>& DataPoints)
{
    UE_LOG(LogTemp, Warning, TEXT("[RangeQuery] HistoryPoints count=%d"), DataPoints.Num());
    for (const auto& P : DataPoints)
    {
        UE_LOG(LogTemp, Warning, TEXT("Point: Time=%f, Value=%f"), P.X, P.Y);
    }
    if (LineChartResult)
    {
        // 直接用 SetChartData 覆蓋舊資料
        LineChartResult->SetChartData(DataPoints);

        UE_LOG(LogTemp, Log, TEXT("LineChart initialized with %d history points"), DataPoints.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("LineChart is nullptr in InitializeChartWithHistory!"));
    }
}