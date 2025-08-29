// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "PrometheusManager.h"

void ULoginWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (LoginButton)
    {
        LoginButton->OnClicked.AddDynamic(this, &ULoginWidget::OnLoginClicked);
    }

    if (IPBox)
    {
        IPBox->OnTextCommitted.AddDynamic(this, &ULoginWidget::OnTextCommitted);
        IPBox->OnTextChanged.AddDynamic(this, &ULoginWidget::OnTextChanged);
    }
    if (UserBox)
    {
        UserBox->OnTextCommitted.AddDynamic(this, &ULoginWidget::OnTextCommitted);
        UserBox->OnTextChanged.AddDynamic(this, &ULoginWidget::OnTextChanged);
    }
    if (PassBox)
    {
        PassBox->OnTextCommitted.AddDynamic(this, &ULoginWidget::OnTextCommitted);
        PassBox->OnTextChanged.AddDynamic(this, &ULoginWidget::OnTextChanged);
    }

    UpdateLoginButtonState();
    if (ErrorText) 
    {
        ErrorText->SetText(FText::GetEmpty());
    }
}

void ULoginWidget::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter)
    {
        if (IsInputValid())
        {
            OnLoginClicked();
        }
        else
        {
            if (ErrorText)
            {
                ErrorText->SetText(FText::FromString(TEXT("請填寫所有欄位才可登入")));
            }
        }
    }
}


void ULoginWidget::OnTextChanged(const FText& Text)
{
    // 每次文字改變就更新按鈕是否可按
    UpdateLoginButtonState();

    // 同時清掉錯誤（若有）
    if (ErrorText && !IsInputValid())
    {
        ErrorText->SetText(FText::GetEmpty());
    }
}

bool ULoginWidget::IsInputValid() const
{
    const FString IP = IPBox ? IPBox->GetText().ToString().TrimStartAndEnd() : FString();
    const FString User = UserBox ? UserBox->GetText().ToString().TrimStartAndEnd() : FString();
    const FString Pass = PassBox ? PassBox->GetText().ToString().TrimStartAndEnd() : FString();

    return !IP.IsEmpty() && !User.IsEmpty() && !Pass.IsEmpty();
}

void ULoginWidget::UpdateLoginButtonState()
{
    bool bValid = IsInputValid();
    if (LoginButton)
    {
        LoginButton->SetIsEnabled(bValid);
    }
}


void ULoginWidget::OnLoginClicked()
{
    if (!IsInputValid())
    {
        if (ErrorText) ErrorText->SetText(FText::FromString(TEXT("請填寫所有欄位")));
        return;
    }

    FString IP = IPBox->GetText().ToString();
    FString User = UserBox->GetText().ToString();
    FString Pass = PassBox->GetText().ToString();

    UE_LOG(LogTemp, Log, TEXT("User Input - IP:%s  User:%s  Pass:%s"), *IP, *User, *Pass);

    APrometheusManager* Manager = Cast<APrometheusManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), APrometheusManager::StaticClass())
    );

    if (Manager)
    {
        Manager->Target_IP = IP;
        Manager->Account = User;
        Manager->Password = Pass;
        Manager->ShowDashboard();

        UE_LOG(LogTemp, Log, TEXT("PrometheusManager updated with user credentials"));
    }
}
