// Fill out your copyright notice in the Description page of Project Settings.


#include "MonitoringItemWidget.h"
#include "PrometheusManager.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "LineChartWidget.h"

void UMonitoringItemWidget::InitializeOptions(const TArray<FString>& Metrics)
{
    MetricComboBox->ClearOptions();
    for (const FString& M : Metrics) MetricComboBox->AddOption(M);
    MetricComboBox->OnSelectionChanged.AddDynamic(this, &UMonitoringItemWidget::OnMetricChanged);

    TypeComboBox->ClearOptions();
    TypeComboBox->AddOption("Raw");
    TypeComboBox->AddOption("Usage%");
    TypeComboBox->OnSelectionChanged.AddDynamic(this, &UMonitoringItemWidget::OnTypeChanged);
}

void UMonitoringItemWidget::OnMetricChanged(FString Selected, ESelectInfo::Type)
{
    SelectedMetric = Selected;

    if (!SelectedType.IsEmpty())
    {
        const FString FinalPromQL = GeneratePromQL(SelectedMetric, SelectedType);
        OnPromQueryGenerated.Broadcast(FinalPromQL, this);
    }
}

void UMonitoringItemWidget::OnTypeChanged(FString Selected, ESelectInfo::Type)
{
    SelectedType = Selected;

    if (!SelectedMetric.IsEmpty())
    {
        const FString FinalPromQL = GeneratePromQL(SelectedMetric, SelectedType);
        OnPromQueryGenerated.Broadcast(FinalPromQL, this);
    }
}


FString UMonitoringItemWidget::GeneratePromQL(const FString& Metric, const FString& Type) const
{
    if (Metric.IsEmpty() || Type.IsEmpty())
        return "";

    if (Type == "Raw")
    {
        return Metric;
    }
    else if (Type == "Usage%" && Metric.Contains("cpu"))
    {
        return FString::Printf(TEXT("sum by (instance) (rate(%s{mode!=\"idle\"}[1m])) * 100"), *Metric);
    }
    else if (Type == "Usage%" && Metric.Contains("memory"))
    {
        return TEXT("(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100");
    }

    // 可選：額外處理無法對應的情況
    UE_LOG(LogTemp, Warning, TEXT("Unknown PromQL mapping for Metric: %s, Type: %s"), *Metric, *Type);
    return "";
}

void UMonitoringItemWidget::TriggerQuery(APrometheusManager* Manager)
{
    if (!Manager) return;

    LastSentPromQL = GeneratePromQL(SelectedMetric, SelectedType);

    // 綁定事件（建議在 Widget 建立時只綁一次）
    Manager->OnQueryResponse.AddDynamic(this, &UMonitoringItemWidget::OnQueryResponseReceived);
    Manager->HandleQuery(LastSentPromQL);
}


void UMonitoringItemWidget::OnQueryResponseReceived(const FString& PromQL, const FString& Result)
{
    UE_LOG(LogTemp, Warning, TEXT("Widget OnQueryResponseReceived - MyLast: %s | Incoming: %s"), *LastSentPromQL, *PromQL);

    if (!ResultText)
    {
        UE_LOG(LogTemp, Error, TEXT("ResultText is nullptr!"));
    }

    if (PromQL == LastSentPromQL && ResultText)
    {
        float FloatValue = FCString::Atof(*Result);
        FString FormattedResult = FString::Printf(TEXT("%.3f"), FloatValue);
        ResultText->SetText(FText::FromString(FormattedResult));
    }
}