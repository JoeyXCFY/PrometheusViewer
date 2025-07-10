#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "MonitoringItemWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPromQueryGenerated, const FString&, PromQL, class UWidget*, TargetWidget);

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

    void InitializeOptions(const TArray<FString>& Metrics);

    FString SelectedMetric;
    FString SelectedType;

    UPROPERTY(BlueprintAssignable)
    FOnPromQueryGenerated OnPromQueryGenerated;

};