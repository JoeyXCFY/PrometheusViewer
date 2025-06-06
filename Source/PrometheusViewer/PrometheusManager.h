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

struct FDataPoint
{
	float Time;
	float Value;

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

	UUserWidget* HUDWidget;

	TArray<FPrometheusQueryInfo> QueryList;

	TMap<FString, TWeakObjectPtr<UTextBlock>> QueryTextMap;

	UFUNCTION()
	void QueryPrometheus(const FString PromQL, const FString Description);
	void OnPrometheusResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void UpdatePrometheus();
	
	FTimerHandle TimerHandle;
protected:
	virtual void BeginPlay() override;
};
