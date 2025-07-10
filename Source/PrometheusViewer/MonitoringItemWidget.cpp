// Fill out your copyright notice in the Description page of Project Settings.


#include "MonitoringItemWidget.h"
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
        FString FinalPromQL;

        if (SelectedType == "Raw")
        {
            FinalPromQL = Selected;
        }
        else if (SelectedType == "Usage%" && Selected.Contains("cpu"))
        {
            FinalPromQL = FString::Printf(TEXT("sum by (instance) (rate(%s{mode!=\"idle\"}[1m])) * 100"), *Selected);
        }
        else if (SelectedType == "Usage%" && Selected.Contains("memory"))
        {
            FinalPromQL = TEXT("(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100");
        }

        OnPromQueryGenerated.Broadcast(FinalPromQL, ResultText);
    }
}

void UMonitoringItemWidget::OnTypeChanged(FString Selected, ESelectInfo::Type)
{
    SelectedType = Selected;
    if (!SelectedMetric.IsEmpty())
    {
        OnMetricChanged(SelectedMetric, ESelectInfo::Type::Direct);
    }
}