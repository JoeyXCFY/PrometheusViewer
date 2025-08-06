#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "MonitoringItemWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPromQueryGenerated, const FString&, PromQL, class UMonitoringItemWidget*, TargetWidget);

UCLASS()
class PROMETHEUSVIEWER_API UMonitoringItemWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    UPROPERTY(meta = (BindWidget)) class UComboBoxString* MetricComboBox;
    UPROPERTY(meta = (BindWidget)) class UComboBoxString* TypeComboBox;
    UPROPERTY(meta = (BindWidget)) class UTextBlock* ResultText;

    UFUNCTION()
    void OnMetricChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    void InitializeOptions(APrometheusManager* Manager);

    UFUNCTION()
    void OnMetricsReady(const TArray<FString>& Metrics);

    FString SelectedMetric;
    FString SelectedType;

    UPROPERTY(BlueprintAssignable)
    FOnPromQueryGenerated OnPromQueryGenerated;

    UTextBlock* GetResultTextBlock() const { return ResultText; }


    UPROPERTY()
    FString LastSentPromQL;

    UFUNCTION()
    FString GeneratePromQL(const FString& Metric, const FString& Type) const;

    UFUNCTION()
    void TriggerQuery(class APrometheusManager* Manager);

    UFUNCTION()
    void OnQueryResponseReceived(const FString& PromQL, const FString& Result);
protected:
    APrometheusManager* ManagerRef;
};