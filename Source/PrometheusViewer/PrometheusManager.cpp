#include "PrometheusManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Http.h"
#include "LineChartWidget.h"
#include "Math/Vector2D.h"
#include "MonitoringItemWidget.h"
#include "DashboardWidget.h"

APrometheusManager::APrometheusManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APrometheusManager::AddDynamicQuery(const FPrometheusQueryInfo& Info)
{
	QueryList.Add(Info);
	QueryTextMap.Add(Info.Description, Info.UITextRef);
}

void APrometheusManager::BeginPlay()
{
	Super::BeginPlay();

	if (LoginWidgetClass)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), LoginWidgetClass);
		if (CurrentWidget)
		{
			CurrentWidget->AddToViewport();
		}
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && CurrentWidget)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(CurrentWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	LoadPromQLMappings();

	GetWorld()->GetTimerManager().SetTimer(AutoQueryTimer, this, &APrometheusManager::ExecuteAutoQueries, 5.0f, true);
}


void APrometheusManager::ShowDashboard()
{
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

	if (DashboardWidgetClass)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), DashboardWidgetClass);
		if (CurrentWidget)
		{
			CurrentWidget->AddToViewport();
		}
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && CurrentWidget)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(CurrentWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void APrometheusManager::HandleQuery(const FString& PromQL)
{
	FString Encoded = FGenericPlatformHttp::UrlEncode(PromQL);
	FString URL = FString::Printf(TEXT("http://%s:9090/api/v1/query?query=%s"), *Target_IP, *Encoded);

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(URL);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Authorization"), "Basic " + FBase64::Encode(Account + ":" + Password));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("PromQL"), PromQL); // 紀錄 PromQL 傳回時對應

	Request->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			//檢查HTTP狀態碼
			if (Resp.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("HTTP Status Code: %d"), Resp->GetResponseCode());
			}
			FString ResultValue = TEXT("N/A");
			FString PromQL = Req->GetHeader(TEXT("PromQL"));

			if (bSuccess && Resp.IsValid())
			{
				TSharedPtr<FJsonObject> Json;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());

				if (FJsonSerializer::Deserialize(Reader, Json))
				{
					const TArray<TSharedPtr<FJsonValue>>* ResultArray;
					if (Json->GetObjectField("data")->TryGetArrayField("result", ResultArray) && ResultArray->Num() > 0)
					{
						const TSharedPtr<FJsonObject>* First;
						if ((*ResultArray)[0]->TryGetObject(First))
						{
							const TArray<TSharedPtr<FJsonValue>>* ValueArray;
							if ((*First)->TryGetArrayField("value", ValueArray) && ValueArray->Num() > 1)
							{
								ResultValue = (*ValueArray)[1]->AsString();
							}
						}
					}
				}
			}
			// 廣播結果讓外部處理
			OnQueryResponse.Broadcast(PromQL, ResultValue);
		}
	);

	Request->ProcessRequest();
	UE_LOG(LogTemp, Warning, TEXT("[HandleQuery] Executing PromQL: %s"), *PromQL);
}

void APrometheusManager::FetchAvailableMetrics()
{

	if (bMetricsFetched)
	{
		OnMetricsFetched.Broadcast(CachedMetrics);
		return;
	}
	FString URL = FString::Printf(TEXT("http://%s:9090/api/v1/label/__name__/values"), *Target_IP);

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(URL);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Authorization"), "Basic " + FBase64::Encode(Account + ":" + Password));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	Request->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			TArray<FString> Metrics;

			if (bSuccess && Resp.IsValid())
			{
				TSharedPtr<FJsonObject> Json;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());

				if (FJsonSerializer::Deserialize(Reader, Json))
				{
					const TArray<TSharedPtr<FJsonValue>>* DataArray;
					if (Json->TryGetArrayField("data", DataArray))
					{
						for (const TSharedPtr<FJsonValue>& Value : *DataArray)
						{
							Metrics.Add(Value->AsString());
						}
					}
				}
			}

			// 呼叫廣播事件或回傳資料
			CachedMetrics = Metrics;
			bMetricsFetched = true;
			OnMetricsFetched.Broadcast(CachedMetrics);
		}
	);

	Request->ProcessRequest();
}


void APrometheusManager::LoadPromQLMappings()
{
	FString JsonPath = FPaths::ProjectContentDir() / TEXT("Config/PromQLMappings.json");
	FString JsonContent;
	if (FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		TSharedPtr<FJsonObject> Root;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);

		if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
		{
			for (auto& MetricPair : Root->Values)
			{
				FString Metric = MetricPair.Key;
				TSharedPtr<FJsonObject> TypeMap = MetricPair.Value->AsObject();
				TMap<FString, FString> InnerMap;

				for (auto& TypePair : TypeMap->Values)
				{
					InnerMap.Add(TypePair.Key, TypePair.Value->AsString());
				}
				PromQLMappings.Add(Metric, InnerMap);
			}
		}
	}
}


FString APrometheusManager::GetPromQLFromMapping(const FString& Metric, const FString& Type) const
{
	if (const TMap<FString, FString>* TypeMap = PromQLMappings.Find(Metric))
	{
		if (const FString* Query = TypeMap->Find(Type))
		{
			return *Query;
		}
	}
	return ""; // 查無資料
}

void APrometheusManager::ExecuteAutoQueries()
{
	for (const FString& Query : RegisteredQueries)
	{
		HandleQuery(Query); // 你原本的查詢函式
		HandleRangeQuery(Query, 300.f, 5.f);
	}
}

void APrometheusManager::RegisterQuery(const FString& PromQL)
{
	if (!RegisteredQueries.Contains(PromQL))
	{
		RegisteredQueries.Add(PromQL);
	}
}

void APrometheusManager::HandleRangeQuery(const FString& PromQL, float RangeSeconds, float StepSeconds)
{
	// 準備 URL
	FString Start = FString::FromInt(FDateTime::UtcNow().ToUnixTimestamp() - RangeSeconds); // 過去 5 分鐘
	FString End = FString::FromInt(FDateTime::UtcNow().ToUnixTimestamp());
	FString StepStr = FString::SanitizeFloat(StepSeconds, 0);

	FString Url = FString::Printf(TEXT("http://%s:9090/api/v1/query_range?query=%s&start=%s&end=%s&step=%s"),
		*Target_IP,
		*FGenericPlatformHttp::UrlEncode(PromQL),
		*Start,
		*End,
		*StepStr);

	UE_LOG(LogTemp, Warning, TEXT("[PrometheusManager] RangeQuery URL = %s"), *Url);
	UE_LOG(LogTemp, Warning, TEXT("UE Now: %s"), *FDateTime::UtcNow().ToString());
	UE_LOG(LogTemp, Warning, TEXT("UE Now Unix: %lld"), FDateTime::UtcNow().ToUnixTimestamp());


	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindLambda(
		[this, PromQL](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (!Response.IsValid() || !EHttpResponseCodes::IsOk(Response->GetResponseCode()))
			{
				UE_LOG(LogTemp, Error, TEXT("[PrometheusManager] RangeQuery failed: %s"), *PromQL);
				UE_LOG(LogTemp, Error, TEXT("[PrometheusManager] RangeQuery failed: %s | Code: %d | Body: %s"),
					*PromQL,
					Response->GetResponseCode(),
					*Response->GetContentAsString());
				return;
			}
			OnRangeQueryResponseReceived(PromQL, Response->GetContentAsString());
		});

	Request->SetURL(Url);
	Request->SetHeader("Content-Type", "application/json");
	Request->SetHeader("Authorization", "Basic " + FBase64::Encode(Account + ":" + Password));
	Request->SetVerb(TEXT("GET"));
	Request->ProcessRequest();
}

void APrometheusManager::OnRangeQueryResponseReceived(const FString& PromQL, const FString& JsonString)
{
	TArray<FVector2D> DataPoints;

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		// 取 data 物件
		const TSharedPtr<FJsonObject>* DataObj;
		if (JsonObject->TryGetObjectField(TEXT("data"), DataObj))
		{
			// 取 result array
			const TArray<TSharedPtr<FJsonValue>>* ResultArray;
			if ((*DataObj)->TryGetArrayField(TEXT("result"), ResultArray))
			{
				for (const TSharedPtr<FJsonValue>& ResultEntry : *ResultArray)
				{
					TSharedPtr<FJsonObject> ResultObj = ResultEntry->AsObject();
					if (!ResultObj.IsValid()) continue;

					// 取 values array
					const TArray<TSharedPtr<FJsonValue>>* ValuesArray;
					if (ResultObj->TryGetArrayField(TEXT("values"), ValuesArray))
					{
						for (const TSharedPtr<FJsonValue>& V : *ValuesArray)
						{
							const TArray<TSharedPtr<FJsonValue>>* PointArray;
							if (V->TryGetArray(PointArray) && PointArray->Num() == 2)
							{
								double Timestamp = (*PointArray)[0]->AsNumber();
								FString ValueStr = (*PointArray)[1]->AsString();

								float Value = FCString::Atof(*ValueStr);

								DataPoints.Add(FVector2D(Timestamp, Value));
							}
						}
					}
				}
			}
		}
	}

	OnRangeQueryResponse.Broadcast(PromQL, DataPoints);

	UE_LOG(LogTemp, Log, TEXT("[Prometheus] RangeQuery %s returned %d points"), *PromQL, DataPoints.Num());
}
