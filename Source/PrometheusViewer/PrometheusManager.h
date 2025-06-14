// Fill out your copyright notice in the Description page of Project Settings.


#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Http.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "PrometheusManager.generated.h"

class UUserWidget;
class UTextBlock;
class ULineChartWidget;

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
	FTimerHandle QueryTimerHandle;
	FTimerHandle RangeQueryTimerHandle;

	UPROPERTY()
	ULineChartWidget* LineChartWidget;
protected:
	virtual void BeginPlay() override;
};
