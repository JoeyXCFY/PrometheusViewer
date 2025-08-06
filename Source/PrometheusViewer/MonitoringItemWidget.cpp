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

    Manager->OnMetricsFetched.AddDynamic(this, &UMonitoringItemWidget::OnMetricsReady);
    Manager->FetchAvailableMetrics(); // 啟動抓取
    TypeComboBox->ClearOptions();
    TypeComboBox->AddOption("Raw");
    TypeComboBox->AddOption("Usage%");
    TypeComboBox->OnSelectionChanged.AddDynamic(this, &UMonitoringItemWidget::OnTypeChanged);
    MetricComboBox->OnSelectionChanged.AddDynamic(this, &UMonitoringItemWidget::OnMetricChanged);
}


void UMonitoringItemWidget::OnMetricsReady(const TArray<FString>& Metrics)
{
    MetricComboBox->ClearOptions();
    for (const FString& M : Metrics)
    {
        MetricComboBox->AddOption(M);
    }
}

void UMonitoringItemWidget::OnMetricChanged(FString Selected, ESelectInfo::Type)
{
    SelectedMetric = Selected;
    UE_LOG(LogTemp, Warning, TEXT("[ComboBox] Metric changed: %s"), *Selected);

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
}

void UMonitoringItemWidget::OnQueryResponseReceived(const FString& PromQL, const FString& Result)
{
    UE_LOG(LogTemp, Warning, TEXT("Widget OnQueryResponseReceived - MyLast: %s | Incoming: %s"), *LastSentPromQL, *PromQL);
    UE_LOG(LogTemp, Warning, TEXT("[QueryResult] PromQL: %s => Result: %s"), *PromQL, *Result);

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