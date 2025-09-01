// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PrometheusGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class PROMETHEUSVIEWER_API UPrometheusGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
