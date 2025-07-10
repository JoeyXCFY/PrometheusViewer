#include "DashboardWidget.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "MonitoringItemWidget.h"
#include "PrometheusManager.h"
#include "EngineUtils.h"
#include "Components/TextBlock.h"

void UDashboardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    /*UTextBlock* TestText = NewObject<UTextBlock>(this);
    TestText->SetText(FText::FromString(TEXT("測試內容是否顯示")));
    MonitorListBox->AddChild(TestText);*/

    if (AddMonitorButton)
    {
        AddMonitorButton->OnClicked.AddDynamic(this, &UDashboardWidget::OnAddMonitorClicked);
    }

    for (TActorIterator<APrometheusManager> It(GetWorld()); It; ++It)
    {
        ManagerRef = *It;
        break;
    }
}

void UDashboardWidget::OnAddMonitorClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Button clicked"));

    if (MonitoringItemWidgetClass)
    {
        UMonitoringItemWidget* NewItem = CreateWidget<UMonitoringItemWidget>(GetWorld(), MonitoringItemWidgetClass);
        if (NewItem)
        {
            UE_LOG(LogTemp, Warning, TEXT("MonitoringItemWidget Added to MonitorListBox"));
            MonitorListBox->AddChild(NewItem);

            TArray<FString> Metrics = { TEXT("node_cpu_seconds_total"), TEXT("node_memory_MemAvailable_bytes") };
            NewItem->InitializeOptions(Metrics);

            NewItem->OnPromQueryGenerated.RemoveDynamic(this, &UDashboardWidget::HandleDynamicPromQL);
            NewItem->OnPromQueryGenerated.AddDynamic(this, &UDashboardWidget::HandleDynamicPromQL);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CreateWidget returned nullptr"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MonitoringItemWidgetClass is null! Please assign it in the editor."));
    }
}

void UDashboardWidget::HandleDynamicPromQL(const FString& PromQL, UWidget* UIWidget)
{
    if (ManagerRef && UIWidget)
    {
        FPrometheusQueryInfo Info;
        Info.PromQL = PromQL;
        Info.Description = "Dynamic";
        Info.UITextRef = Cast<UTextBlock>(UIWidget);
        ManagerRef->AddDynamicQuery(Info);
    }
}