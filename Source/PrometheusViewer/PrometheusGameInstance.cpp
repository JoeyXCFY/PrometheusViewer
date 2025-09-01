// Fill out your copyright notice in the Description page of Project Settings.


#include "PrometheusGameInstance.h"
#include "GameFramework/GameUserSettings.h"

void UPrometheusGameInstance::Init()
{
    Super::Init();

    if (GEngine)
    {
        UGameUserSettings* Settings = GEngine->GetGameUserSettings();
        if (Settings)
        {
            Settings->SetFullscreenMode(EWindowMode::Windowed);
            Settings->SetScreenResolution(FIntPoint(1280, 720));
            Settings->ApplySettings(false);
        }
    }
}
