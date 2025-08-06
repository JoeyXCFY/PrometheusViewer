#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Http.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Delegates/DelegateCombinations.h"
#include "PrometheusManager.generated.h"


class UUserWidget;
class UTextBlock;
class ULineChartWidget;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPrometheusQueryResponse, const FString&, PromQL, const FString&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMetricsFetchedDelegate, const TArray<FString>&, Metrics);


USTRUCT(BlueprintType)
struct FPromQLMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Metric;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PromQL;
};

USTRUCT()
struct FMonitoringRequest
{
	GENERATED_BODY()

	FString MetricName;
	FString DisplayType; // "Raw" 或 "Usage%"
	class UMonitoringItemWidget* WidgetRef;
};

USTRUCT(BlueprintType)
struct FDataPoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	float Time;

	UPROPERTY(BlueprintReadWrite)
	float Value;
	
	FDataPoint()
		: Time(0), Value(0)
	{}

	FDataPoint(float InTime, float InValue)	: Time(InTime), Value(InValue) {}
};

USTRUCT(BlueprintType)
struct FPrometheusQueryInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FString PromQL;

	UPROPERTY()
	FString Description;

	UPROPERTY()
	TWeakObjectPtr<UTextBlock> UITextRef;
};


USTRUCT(BlueprintType)
struct FPrometheusRangeQueryInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FString Description;

	UPROPERTY()
	FString PromQL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FColor LineColor;

	UPROPERTY()
	ULineChartWidget* LineChartWidgetRef;
};



UCLASS()
class PROMETHEUSVIEWER_API APrometheusManager : public AActor
{
	GENERATED_BODY()
	
public:	
	APrometheusManager();
	UPROPERTY(EditAnywhere, Category = "PrometheusManage")
	FString Target_IP;
	UPROPERTY(EditAnywhere, Category = "PrometheusManage")
	FString Account;
	UPROPERTY(EditAnywhere, Category = "PrometheusManage")
	FString Password;
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget>WidgetClass;

	TArray<FPrometheusRangeQueryInfo> RangeQueryList;

	UUserWidget* HUDWidget;

	TArray<FPrometheusQueryInfo> QueryList;

	TMap<FString, TWeakObjectPtr<ULineChartWidget>> LineChartMap;
	TMap<FString, TWeakObjectPtr<UTextBlock>> QueryTextMap;

	UFUNCTION()
	void QueryPrometheus(const FPrometheusQueryInfo& Info);
	void OnPrometheusResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void UpdatePrometheus();
	void UpdateRangeMetrics();
	void OnQueryRangeResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void QueryRangePrometheus(const FPrometheusRangeQueryInfo& Info);
	void FetchAvailableMetrics();

	FTimerHandle QueryTimerHandle;
	FTimerHandle RangeQueryTimerHandle;
	void HandleQuery(const FString& PromQL);

	UPROPERTY(BlueprintAssignable)
	FOnPrometheusQueryResponse OnQueryResponse;

	UPROPERTY(BlueprintAssignable, Category = "Prometheus")
	FMetricsFetchedDelegate OnMetricsFetched;

	UPROPERTY()
	TArray<FString> MetricNameList;

	UPROPERTY()
	ULineChartWidget* LineChartWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Prometheus")
	UDataTable* PromQLMappingTable;

	void AddDynamicQuery(const FPrometheusQueryInfo& Info);

	TMap<FString, TMap<FString, FString>> PromQLMappings;

	void LoadPromQLMappings();

	FString GetPromQLFromMapping(const FString& Metric, const FString& Type) const;

protected:
	virtual void BeginPlay() override;
	TArray<FMonitoringRequest> PendingMonitoringRequests;
};
