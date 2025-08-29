// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoginWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROMETHEUSVIEWER_API ULoginWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* IPBox;

    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* UserBox;

    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* PassBox;

    UPROPERTY(meta = (BindWidget))
    class UButton* LoginButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ErrorText;

    // 新增方法宣告
    UFUNCTION()
    void OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnTextChanged(const FText& Text);

    bool IsInputValid() const;
    void UpdateLoginButtonState();
protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void OnLoginClicked();
};
