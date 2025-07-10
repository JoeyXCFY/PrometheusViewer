#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DashboardWidget.generated.h"

class UButton;
class UScrollBox;
class UMonitoringItemWidget;
class UTextBlock;

UCLASS()
class PROMETHEUSVIEWER_API UDashboardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget)) class UButton* AddMonitorButton;
    UPROPERTY(meta = (BindWidget)) class UScrollBox* MonitorListBox;

    UFUNCTION() void OnAddMonitorClicked();

    UPROPERTY(EditAnywhere) TSubclassOf<class APrometheusManager> ManagerClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    TSubclassOf<UMonitoringItemWidget> MonitoringItemWidgetClass;

    UFUNCTION()
    void HandleDynamicPromQL(const FString& PromQL, UWidget* UIWidget);

    class APrometheusManager* ManagerRef = nullptr;

    virtual void NativeConstruct() override;
};