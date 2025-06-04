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
class PROMETHEUSVIWER_API APrometheusManager : public AActor
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
	UTextBlock* MemoryTextRef;
	UTextBlock* CPUTextRef;
	
	UFUNCTION()
	void QueryPrometheus(const FString PromQL, const FString Description);
	void OnPrometheusResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void UpdatePrometheus();
	
	FTimerHandle TimerHandle;
protected:
	virtual void BeginPlay() override;
};
