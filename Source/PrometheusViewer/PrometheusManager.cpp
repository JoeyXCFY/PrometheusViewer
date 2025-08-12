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

	if (WidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}

	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(HUDWidget->TakeWidget()); 
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	LoadPromQLMappings();

	FPrometheusQueryInfo MemoryQuery;
	MemoryQuery.PromQL = "(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100";
	MemoryQuery.Description = "Memory";
	MemoryQuery.UITextRef = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("MemoryUsage")));
	QueryList.Add(MemoryQuery);
	QueryTextMap.Add(MemoryQuery.Description, MemoryQuery.UITextRef);

	FPrometheusQueryInfo CPUQuery;
	CPUQuery.PromQL = "sum by (instance) (rate(node_cpu_seconds_total{mode!=\"idle\"}[1m])) * 100";
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

	FPrometheusRangeQueryInfo CPURangeQuery;
	CPURangeQuery.PromQL = "sum by (instance) (rate(node_cpu_seconds_total{mode!=\"idle\"}[1m])) * 100";
	CPURangeQuery.Description = "CPURange";
	CPURangeQuery.LineColor = FColor::Green;
	CPURangeQuery.LineChartWidgetRef = Cast<ULineChartWidget>(HUDWidget->GetWidgetFromName(TEXT("CPULineChart")));
	RangeQueryList.Add(CPURangeQuery);
	LineChartMap.Add(CPURangeQuery.Description, CPURangeQuery.LineChartWidgetRef);

	//GetWorld()->GetTimerManager().SetTimer(QueryTimerHandle, this, &APrometheusManager::UpdatePrometheus, 10.0f, true, 0.0f);
	//GetWorld()->GetTimerManager().SetTimer(RangeQueryTimerHandle, this, &APrometheusManager::UpdateRangeMetrics, 10.0f, true, 0.0f);
	GetWorld()->GetTimerManager().SetTimer(AutoQueryTimer, this, &APrometheusManager::ExecuteAutoQueries, 5.0f, true);
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
	FString Start = FString::FromInt(FDateTime::UtcNow().ToUnixTimestamp() - 3600); // 過去 5 分鐘
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
				TMap<double, TArray<float>> AggregatedData;

				for (const TSharedPtr<FJsonValue>& ResultValue : *Results)
				{
					const TSharedPtr<FJsonObject>* ResultObject;
					if (ResultValue->TryGetObject(ResultObject))
					{
						const TArray<TSharedPtr<FJsonValue>>* Values;
						if ((*ResultObject)->TryGetArrayField(TEXT("values"), Values))
						{
							for (const TSharedPtr<FJsonValue>& ValuePair : *Values)
							{
								const TArray<TSharedPtr<FJsonValue>>* PairArray;
								if (ValuePair->TryGetArray(PairArray) && PairArray->Num() == 2)
								{
									double Timestamp = (*PairArray)[0]->AsNumber();
									FString ValueStr = (*PairArray)[1]->AsString();
									float Value = FCString::Atof(*ValueStr);

									AggregatedData.FindOrAdd(Timestamp).Add(Value);
								}
							}
						}
					}
				}

				// 將結果轉為 FVector2D 陣列，合併後取平均 
				TArray<FVector2D> ConvertedData;
				for (const TPair<double, TArray<float>>& Entry : AggregatedData)
				{
					double Timestamp = Entry.Key;
					const TArray<float>& Values = Entry.Value;

					if (Values.Num() > 0)
					{
						float Sum = 0.0f;
						for (float V : Values)
						{
							Sum += V;
						}
						float Avg = Sum / Values.Num();
						ConvertedData.Add(FVector2D(Timestamp, Avg));
					}
				}

				// 時間排序（確保圖表點不亂序）
				ConvertedData.Sort([](const FVector2D& A, const FVector2D& B) {
					return A.X < B.X;
					});

				// 更新圖表
				if (FoundLineChartRef && FoundLineChartRef->IsValid())
				{
					FoundLineChartRef->Get()->SetChartData(ConvertedData);
				}

				/*for (const FVector2D& Point : ConvertedData)
				{
					UE_LOG(LogTemp, Log, TEXT("Merged Time: %f, Avg Value: %f"), Point.X, Point.Y);
				}*/
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response"));
	}
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
	}
}

void APrometheusManager::RegisterQuery(const FString& PromQL)
{
	if (!RegisteredQueries.Contains(PromQL))
	{
		RegisteredQueries.Add(PromQL);
	}
}
