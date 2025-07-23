// Fill out your copyright notice in the Description page of Project Settings.


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
	void FetchAllMetricNames();
	void OnReceiveMetricList(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	FTimerHandle QueryTimerHandle;
	FTimerHandle RangeQueryTimerHandle;
	void HandleQuery(const FString& PromQL);

	UPROPERTY(BlueprintAssignable)
	FOnPrometheusQueryResponse OnQueryResponse;

	UPROPERTY()
	TArray<FString> MetricNameList;

	UPROPERTY()
	ULineChartWidget* LineChartWidget;

	void AddDynamicQuery(const FPrometheusQueryInfo& Info);

protected:
	virtual void BeginPlay() override;
	TArray<FMonitoringRequest> PendingMonitoringRequests;
};
