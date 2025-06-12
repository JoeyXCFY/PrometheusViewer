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

APrometheusManager::APrometheusManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APrometheusManager::BeginPlay()
{
	Super::BeginPlay();
	Target_IP = "172.16.100.19";
	Account = "admin";
	Password = "admin";

	if (WidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}

	}

	FPrometheusQueryInfo MemoryQuery;
	MemoryQuery.PromQL = "(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100";
	MemoryQuery.Description = "Memory";
	MemoryQuery.UITextRef = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("MemoryUsage")));
	QueryList.Add(MemoryQuery);
	QueryTextMap.Add(MemoryQuery.Description, MemoryQuery.UITextRef);

	FPrometheusQueryInfo CPUQuery;
	CPUQuery.PromQL = "100 - (avg by(instance)(rate(node_cpu_seconds_total{ mode = \"idle\" } [1m] )) * 100)";
	CPUQuery.Description = "CPU";
	CPUQuery.UITextRef = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("CPUUsage")));
	QueryList.Add(CPUQuery);
	QueryTextMap.Add(CPUQuery.Description, CPUQuery.UITextRef);

	FPrometheusRangeQueryInfo MemoryRangeQuery;
	MemoryRangeQuery.PromQL = "(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100";
	MemoryRangeQuery.Description = "MemoryRange";
	MemoryRangeQuery.LineColor = FColor::Green;
	MemoryRangeQuery.LineChartWidgetRef = Cast<ULineChartWidget>(HUDWidget->GetWidgetFromName(TEXT("MemoryLineChart")));
	RangeQueryList.Add(MemoryRangeQuery);
	LineChartMap.Add(MemoryRangeQuery.Description, MemoryRangeQuery.LineChartWidgetRef);

	GetWorld()->GetTimerManager().SetTimer(QueryTimerHandle, this, &APrometheusManager::UpdatePrometheus, 5.0f, true, 0.0f);
	GetWorld()->GetTimerManager().SetTimer(RangeQueryTimerHandle, this, &APrometheusManager::UpdateRangeMetrics, 10.0f, true, 0.0f);
}


void APrometheusManager::UpdatePrometheus()
{
	for (const FPrometheusQueryInfo& Info : QueryList)
	{
		QueryPrometheus(Info);
	}
}

void APrometheusManager::UpdateRangeMetrics()
{
	for (const FPrometheusRangeQueryInfo& Info : RangeQueryList)
	{
		QueryRangePrometheus(Info);
	}
}

void APrometheusManager::QueryPrometheus(const FPrometheusQueryInfo& Info)
{
	FString EncodedQuery = FGenericPlatformHttp::UrlEncode(Info.PromQL);
	FString URL = "http://" + Target_IP + ":9090/api/v1/query?query=" + EncodedQuery;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetHeader("Description", Info.Description);

	//請求存取憑證
	FString Credentials = Account + ":" + Password;
	FString EncodedCredentials = FBase64::Encode(Credentials);
	Request->SetHeader("Authorization", "Basic " + EncodedCredentials);
	Request->SetURL(URL);
	//Prometheus HTTP API Query URL設定
	Request->SetVerb("GET");
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	//請求Request
	Request->OnProcessRequestComplete().BindUObject(this, &APrometheusManager::OnPrometheusResponseReceived);
	Request->ProcessRequest();
}

void APrometheusManager::QueryRangePrometheus(const FPrometheusRangeQueryInfo& Info)
{
	FString Start = FString::FromInt(FDateTime::UtcNow().ToUnixTimestamp() - 300); // 過去 5 分鐘
	FString End = FString::FromInt(FDateTime::UtcNow().ToUnixTimestamp());
	FString Step = "10"; // 每 10 秒一筆

	FString URL = FString::Printf(TEXT("http://%s:9090/api/v1/query_range?query=%s&start=%s&end=%s&step=%s"),
		*Target_IP,
		*FGenericPlatformHttp::UrlEncode(Info.PromQL),
		*Start,
		*End,
		*Step);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(URL);
	Request->SetVerb("GET");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetHeader("Authorization", "Basic " + FBase64::Encode(Account + ":" + Password));
	Request->SetHeader("Description", Info.Description); 

	Request->OnProcessRequestComplete().BindUObject(this, &APrometheusManager::OnQueryRangeResponse);
	Request->ProcessRequest();
}

void APrometheusManager::OnPrometheusResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	//檢查請求是否成功
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to receive response from Prometheus."));
		return;
	}
	//檢查HTTP狀態碼
	if (Response.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("HTTP Status Code: %d"), Response->GetResponseCode());
	}

	//Response內容
	FString ResponseString = Response->GetContentAsString();
	UE_LOG(LogTemp, Log, TEXT("Prometheus response received:\n%s"), *ResponseString);

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	TSharedPtr<FJsonObject> JsonObject;

	FString Description = Request->GetHeader("Description");
	TWeakObjectPtr<UTextBlock>* FoundTextRef = QueryTextMap.Find(Description);
	if (!FoundTextRef || !FoundTextRef->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("No TextBlock found for %s"), *Description);
		return;
	}

	//解析JSON
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		//檢查狀態
		FString Status = JsonObject->GetStringField(TEXT("status"));
		if (Status == "success")
		{
			UE_LOG(LogTemp, Log, TEXT("Query succeeded."));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Query returned non-success status: %s"), *Status);
		}
		//逐步解構JSON
		const TSharedPtr<FJsonObject>* Data;
		if (JsonObject->TryGetObjectField(TEXT("data"), Data))
		{
			const TArray<TSharedPtr<FJsonValue>>* ResultArray;
			if ((*Data)->TryGetArrayField(TEXT("result"), ResultArray) && ResultArray->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* FirstResult;
				if ((*ResultArray)[0]->TryGetObject(FirstResult))
				{
					const TArray<TSharedPtr<FJsonValue>>* ValueArray;
					if ((*FirstResult)->TryGetArrayField(TEXT("value"), ValueArray) && ValueArray->Num() > 1)
					{
						FString ValueStr = (*ValueArray)[1]->AsString();
						float Value = FCString::Atof(*ValueStr);
						FString ValueStrFormatted = FString::Printf(TEXT("%.3f%%"), Value);

						//更新UI
						if (FoundTextRef) {
							FoundTextRef->Get()->SetText(FText::FromString(ValueStrFormatted));
						}
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse Prometheus JSON response."));
	}
}

void APrometheusManager::OnQueryRangeResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Request failed or response invalid"));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FString Description = Request->GetHeader("Description");
	TWeakObjectPtr<ULineChartWidget>* FoundLineChartRef = LineChartMap.Find(Description);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		const TSharedPtr<FJsonObject>* DataObject;
		if (JsonObject->TryGetObjectField(TEXT("data"), DataObject))
		{
			const TArray<TSharedPtr<FJsonValue>>* Results;
			if ((*DataObject)->TryGetArrayField(TEXT("result"), Results) && Results->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* FirstResult;
				if ((*Results)[0]->TryGetObject(FirstResult))
				{
					const TArray<TSharedPtr<FJsonValue>>* Values;
					if ((*FirstResult)->TryGetArrayField(TEXT("values"), Values))
					{
						TArray<FDataPoint> DataPoints;

						for (const TSharedPtr<FJsonValue>& ValuePair : *Values)
						{
							const TArray<TSharedPtr<FJsonValue>>* PairArray;
							if (ValuePair->TryGetArray(PairArray) && PairArray->Num() == 2)
							{
								double Timestamp =(* PairArray)[0]->AsNumber();
								FString ValueStr =(* PairArray)[1]->AsString();
								float Value = FCString::Atof(*ValueStr);

								DataPoints.Add(FDataPoint(Timestamp, Value));
							}
						}
						TArray<FVector2D> ConvertedData;

						for (const FDataPoint& Point : DataPoints)
						{
							ConvertedData.Add(FVector2D(Point.Time, Point.Value));
						}

						if (LineChartMap.Contains(Description) && FoundLineChartRef->IsValid())
						{
							// 更新LineChartWidget
							FoundLineChartRef->Get()->SetChartData(ConvertedData);
						}

						// Debug log
						for (const FDataPoint& Point : DataPoints)
						{
							UE_LOG(LogTemp, Log, TEXT("Time: %f, Value: %f"), Point.Time, Point.Value);
						}

					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response"));
	}
}