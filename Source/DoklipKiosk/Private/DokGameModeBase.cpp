// Fill out your copyright notice in the Description page of Project Settings.


#include "DokGameModeBase.h"
#include "DokGameInstance.h"
#include "HttpModule.h"
#include "Chaos/ArrayND.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/KismetSystemLibrary.h"

ADokGameModeBase::ADokGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADokGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	
	GI = Cast<UDokGameInstance>(GetGameInstance());

	// 시작 state 설정
	DH_GMP_state = EDHGameModePlayState::GMP_SIT;

	DH_GM_state = EDHGameModeState::GM_KIM;
}

void ADokGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ADokGameModeBase::StartSocketOn()
{
	if(GI != nullptr)
	{
		GI->StartSocket();
		
		UE_LOG(LogTemp, Warning, TEXT("start socket"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("gi is null"));
	}
}

void ADokGameModeBase::DH_GM_CHANGE()
{
	switch (DH_GM_state)
	{
	case EDHGameModeState::GM_KIM:
		GoKim();
		break;
	case EDHGameModeState::GM_AN:
		GoAn();
		break;
	case EDHGameModeState::GM_YUN:
		GoYun();
		break;
	}
}

void ADokGameModeBase::SetDH_GM_State(EDHGameModeState next)
{
	DH_GM_state = next;

	DH_GM_CHANGE();
}

void ADokGameModeBase::GoKim()
{
	SetAnARKitOff();

	SetYunARKitOff();
	
	CheckChangeStatus();
}

void ADokGameModeBase::GoAn()
{
	SetKimARKitOff();
	
	SetYunARKitOff();
	
	CheckChangeStatus();
}

void ADokGameModeBase::GoYun()
{
	SetKimARKitOff();
	
	SetAnARKitOff();
	
	CheckChangeStatus();
}

void ADokGameModeBase::StartAudio2faceOn()
{
	// 시작 설정 시작
	CheckStatus();
}

//-----------------------------------------------------------------------------------------audio2face 시작 세팅
void ADokGameModeBase::CheckStatus()
{
	// audio2face status check
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseCheckStatus);
	Request->SetURL("http://localhost:8011/status");
	Request->SetVerb("Get");
	Request->SetHeader("accept", "application/json");
	Request->ProcessRequest();
}

// audio2face response log
void ADokGameModeBase::ResponseCheckStatus(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	
	UE_LOG(LogTemp, Warning, TEXT("audio2face status %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{
		SetKimARKitOn();
			
		SetKimUSD();
	}
	else
	{
		// 상태 확인 재시도
		FTimerHandle time;
		GetWorldTimerManager().SetTimer(time, this, &ADokGameModeBase::CheckStatus, 5.0f, false);
	}
}

void ADokGameModeBase::CheckChangeStatus()
{
	// audio2face status check
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseCheckChangeStatus);
	Request->SetURL("http://localhost:8011/status");
	Request->SetVerb("Get");
	Request->SetHeader("accept", "application/json");
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseCheckChangeStatus(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	
	UE_LOG(LogTemp, Warning, TEXT("audio2face status %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{
		// 인물별 usd 설정으로 넘어가기
		if(DH_GM_state == EDHGameModeState::GM_KIM)
		{
			SetKimARKitOn();
			
			SetKimUSD();
		}
		else if(DH_GM_state == EDHGameModeState::GM_AN)
		{
			SetAnARKitOn();
			
			SetAnUSD();
		}
		else if(DH_GM_state == EDHGameModeState::GM_YUN)
		{
			SetYunARKitOn();
			
			SetYunUSD();
		}
	}
	else
	{
		// 상태 확인 재시도
		FTimerHandle time;
		GetWorldTimerManager().SetTimer(time, this, &ADokGameModeBase::CheckChangeStatus, 5.0f, false);

		// 오류 mqtt 쏘기
		UE_LOG(LogTemp, Display, TEXT("audio2face error~!!!! mqtt 9 push"));
	
		CallBackError();
	}
}

void ADokGameModeBase::SetKimUSD()
{	
	// usd setting
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("file_name", "omniverse://localhost/Users/kiosk/kimgu.usd");
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseSetKimUSD);
	Request->SetURL("http://localhost:8011/A2F/USD/Load");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseSetKimUSD(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	
	UE_LOG(LogTemp, Warning, TEXT("ResponseSetUSD %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		FTimerHandle time;
		GetWorldTimerManager().SetTimer(time, this, &ADokGameModeBase::SetActiveLiveLink, 2.0f, false);
	}
	else
	{
		// 오류 mqtt 쏘기
		
		UE_LOG(LogTemp, Display, TEXT("audio2face error~!!!! mqtt 9 push"));
		
		CallBackError();
	}
}

void ADokGameModeBase::SetAnUSD()
{	
	// usd setting
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("file_name", "omniverse://localhost/Users/kiosk/anjunggeun.usd");
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseSetAnUSD);
	Request->SetURL("http://localhost:8011/A2F/USD/Load");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseSetAnUSD(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	
	UE_LOG(LogTemp, Warning, TEXT("ResponseSetUSD %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		FTimerHandle time;
		GetWorldTimerManager().SetTimer(time, this, &ADokGameModeBase::SetActiveLiveLink, 2.0f, false);
	}
	else
	{
		// 오류 mqtt 쏘기

		UE_LOG(LogTemp, Display, TEXT("audio2face error~!!!! mqtt 9 push"));
		
		CallBackError();
	}
}

void ADokGameModeBase::SetYunUSD()
{	
	// usd setting
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("file_name", "omniverse://localhost/Users/kiosk/yunbonggil.usd");
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseSetYunUSD);
	Request->SetURL("http://localhost:8011/A2F/USD/Load");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseSetYunUSD(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	
	UE_LOG(LogTemp, Warning, TEXT("ResponseSetUSD %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		FTimerHandle time;
		GetWorldTimerManager().SetTimer(time, this, &ADokGameModeBase::SetActiveLiveLink, 2.0f, false);
	}
	else
	{
		// 오류 mqtt 쏘기

		UE_LOG(LogTemp, Display, TEXT("audio2face error~!!!! mqtt 9 push"));
		
		CallBackError();
	}
}

void ADokGameModeBase::SetActiveLiveLink()
{
	// post active live link setting
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("node_path", "/World/audio2face/StreamLivelink");
	RequestObj->SetStringField("value", "true");

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseSetActiveLiveLink);
	Request->SetURL("http://localhost:8011/A2F/Exporter/ActivateStreamLivelink");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseSetActiveLiveLink(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseActiveLiveLink %s"), *Response->GetContentAsString());

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		// 시작 세팅 끝
		UE_LOG(LogTemp, Warning, TEXT("start setting end"));}
	else
	{
		// 오류 mqtt 쏘기

		UE_LOG(LogTemp, Display, TEXT("audio2face error~!!!! mqtt 9 push"));
		
		CallBackError();
	}
}

// mqtt 통신 시작 후 립싱크 시작
void ADokGameModeBase::SetTrack()
{
	// audio track setting
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");
	RequestObj->SetStringField("file_name", F_FileString);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseSetTrack);
	Request->SetURL("http://localhost:8011/A2F/Player/SetTrack");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseSetTrack(FHttpRequestPtr Request, FHttpResponsePtr Response,
									   bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseSetTrack %s"), *Response->GetContentAsString());

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		GetRange();
	}
	else
	{
		// 오디오 없음

		UE_LOG(LogTemp, Display, TEXT("audio emty or audio2face error~!!!! mqtt 9 push"));

		CallBackError();
		
		NotExistAudio();
	}
}

void ADokGameModeBase::GetRange()
{
	// get audio range
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");
	
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseGetRange);
	Request->SetURL("http://localhost:8011/A2F/Player/GetRange");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseGetRange(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseGetRange %s"), *Response->GetContentAsString());

	// json 구조체 읽어서 오디오 길이 float에 담기
	TSharedPtr<FJsonObject> object = ResponseObj->GetObjectField(TEXT("result"));
	TArray<TSharedPtr<FJsonValue>> objectArray = object->GetArrayField(TEXT("default"));

	if(objectArray[1] != nullptr)
	{
		auto idx = objectArray[1]->AsNumber();

		audioRange = Chaos::ConvertDoubleToFloat(idx);
	}
	else
	{
		audioRange = 0;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ResponseGetRange %f"), audioRange);

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		Play();
	}
}

void ADokGameModeBase::Play()
{
	// post audio play
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponsePlay);
	Request->SetURL("http://localhost:8011/A2F/Player/Play");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponsePlay(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponsePlay %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{
		// type이 answer일 경우
		if(F_TypeString == "answer")
		{
			// 정상 답변 일때
			if(bCheckNotAnswer != true)
			{
				if(F_GestureString == TEXT("입식") || F_GestureString == "")
				{
					// audio 길이만큼 시간이 흐른 다음
					GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackMQTT, audioRange, false);
				}
				else
				{
					// audio 길이만큼 시간이 흐른 다음
					GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackMQTT, audioRange, false);
					 	
					// gesture 플레이 하기
					SetGesture();
				}
			}
			// 정상 답변 아닐때
			else
			{
				bCheckNotAnswer = false;

				// audio 길이만큼 시간이 흐른 다음 
				GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackMQTT, audioRange, false);

				// gesture 플레이 하기
				SetGesture();
			}
		}
		// 그 외의 경우
		else if(F_TypeString == "activate")
		{
			if(F_StateString == "1007")
			{
				// audio 길이만큼 시간이 흐른 다음 
				GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackMQTT, audioRange, false);

				SetGesture();
			}
			else
			{
				// audio 길이만큼 시간이 흐른 다음 
				GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::EndFrontEndCallBackMQTT, audioRange, false);

				SetGesture();
			}
		}
		else if(F_TypeString == "deactivate")
		{
			// audio 길이만큼 시간이 흐른 다음 
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::EndBackEndCallBackMQTT, audioRange, false);

			SetGesture();
		}
		else if(F_TypeString == "quiz")
		{
			// audio 길이만큼 시간이 흐른 다음 
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackQuiz, audioRange, false);

			SetGesture();
		}
	}
}

void ADokGameModeBase::GetTime()
{
	// post audio gettime
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseGetTime);
	Request->SetURL("http://localhost:8011/A2F/Player/GetTime");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseGetTime(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponsePause %s"), *Response->GetContentAsString());
	
	float time = ResponseObj->GetNumberField(TEXT("result"));
	
	if(bConnectedSuccessfully != false)
	{
		// 오디오 시간 지금 시간이 다르면(오디오가 안끝났으면)
		if(time != audioRange)
		{
			SetTrack0000();
		}
	}
}

void ADokGameModeBase::SetTrack0000()
{
	// audio track setting
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");
	RequestObj->SetStringField("file_name", "0000.wav");

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseSetTrack0000);
	Request->SetURL("http://localhost:8011/A2F/Player/SetTrack");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseSetTrack0000(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseSetTrack %s"), *Response->GetContentAsString());

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		Play0000();
	}
}

void ADokGameModeBase::Play0000()
{
	// post audio play
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponsePlay0000);
	Request->SetURL("http://localhost:8011/A2F/Player/Play");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponsePlay0000(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponsePlay %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{
		UE_LOG(LogTemp, Warning, TEXT("play 0000 sucess"));
	}
}

void ADokGameModeBase::Pause()
{
	// post audio pause
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponsePause);
	Request->SetURL("http://localhost:8011/A2F/Player/Pause");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponsePause(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponsePause %s"), *Response->GetContentAsString());

	if(bConnectedSuccessfully != false)
	{
		// 게임 인스턴스 사운드 정지
	}
}

void ADokGameModeBase::PauseAndTimerReset()
{
	UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(GetWorld(),AudioTimer);
	
	Pause();
}

void ADokGameModeBase::SetGesture()
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *F_GestureString);

	UE_LOG(LogTemp, Warning, TEXT("play gesture"));

	UE_LOG(LogTemp, Warning, TEXT("bCheckTwo is : %s"), bCheckTwo ? TEXT("true") : TEXT("false"));
	
	if(bCheckTwo != true)
	{
		bCheckTwo = true;
		
		if(F_GestureString == TEXT("한팔"))
		{
			PlayOnehand();
		}
		else if (F_GestureString == TEXT("양팔"))
		{
			PlayTwohand();
		}
		else if (F_GestureString == TEXT("지연1"))
		{
			PlayChinrest();
		}
		// else if (F_GestureString == TEXT("지연2"))
		// {
		// 	PlayArmacross();
		// }
		else if (F_GestureString == TEXT("불용"))
		{
			PlayInsoluble();
		}
		else if (F_GestureString == TEXT("인사"))
		{
			PlayHello();
		}
		else if (F_GestureString == TEXT("커스텀"))
		{
			PlayCustom();
		}
		else if (F_GestureString == TEXT("뒷짐"))
		{
			PlayBack();
		}
		else if (F_GestureString == TEXT("경청"))
		{
			PlayHear();
		}
		else if (F_GestureString == TEXT("인식"))
		{
			PlayRecognize();
		}
		else if (F_GestureString == TEXT("한팔_강조"))
		{
			PlayOneCheck();
		}
		else if (F_GestureString == TEXT("양팔_강조"))
		{
			PlayTwoCheck();
		}
		else
		{
			CheckTwo();
		}
	}
}

void ADokGameModeBase::CheckTwo()
{
	bCheckTwo = false;
}

// mqtt 통신
void ADokGameModeBase::CallReceivedMQTT(FString received)
{
	// mqtt 통신 변수에 동기화
	TSharedRef<TJsonReader<TCHAR>> reader = TJsonReaderFactory<TCHAR>::Create(received);
	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject());
	FJsonSerializer::Deserialize(reader, jsonObject);

	// state 분별
	FString stateString = jsonObject->GetStringField(TEXT("state"));
	UE_LOG(LogTemp, Warning, TEXT("state : %s"), *stateString);
		
	TSharedPtr<FJsonObject> object = jsonObject->GetObjectField(TEXT("payload"));
	
	// type, filename, gesture
	FString type_String = object->GetStringField(TEXT("type"));
	FString answer_type_String = object->GetStringField(TEXT("answer_type"));
	FString filename_String = object->GetStringField(TEXT("filename"));
	FString gesture_String = object->GetStringField(TEXT("gesture"));

	if(stateString == "0")
	{
		DH_GMP_state = EDHGameModePlayState::GMP_SIT;

		// state 저장
		F_StateString = stateString;
		
		State0();
	}
	else if(stateString == "1")
	{
		PauseAndTimerReset();
		
		// state 저장
		F_StateString = stateString;
		
		State1();
	}
	else if(stateString == "9")
	{
		PauseAndTimerReset();

		// state 저장
		F_StateString = stateString;

		State9();
	}
	// state 활성화
	else if(stateString == "100")
	{
		PauseAndTimerReset();
		
		// state 저장
		F_StateString = stateString;
		
		CharacterSync();
	}
	else if(stateString == "102")
	{
		PauseAndTimerReset();
		
		// 변수에 저장
		F_TypeString = type_String;
		F_FileString = filename_String;
		F_GestureString = gesture_String;
		
		InputMoveFront();
	}
	// state 비활성화
	else if(stateString == "201")
	{
		PauseAndTimerReset();
		
		// 변수에 저장
		F_TypeString = type_String;
		F_FileString = filename_String;
		F_GestureString = gesture_String;
		
		InputMoveBack();
	}
	else if(stateString == "300")
	{
		State300();
	}
	// state 인물 발화
	else if(stateString == "303" || stateString == "400")
	{
		PauseAndTimerReset();
		
		if(answer_type_String == "violation" || answer_type_String == "banned_word")
		{
			// 변수에 저장
			F_TypeString = type_String;
			F_StateString = stateString;
			F_FileString = filename_String;
			F_GestureString = gesture_String;
				
			SetNotAnswer();
		}
		// 정상 대화
		else
		{
			// 변수에 저장
			F_TypeString = type_String;
			F_StateString = stateString;
			F_FileString = filename_String;
			F_GestureString = gesture_String;
				
			SetAnswer();
		}
	}
	// 카메라
	else if(stateString == "601")
	{
		// state 저장
		F_StateString = stateString;

		State601();
	}
	else if(stateString == "602")
	{
		// state 저장
		F_StateString = stateString;

		State602();
	}
	else if(stateString == "603")
	{
		// state 저장
		F_StateString = stateString;

		State603();
	}
	else if(stateString == "607")
	{
		// state 저장
		F_StateString = stateString;

		State607();
	}
	// 성향 분석
	else if(stateString == "702")
	{
		PauseAndTimerReset();
		
		// state 저장
		F_StateString = stateString;
		
		int32 getIndex = object->GetNumberField(TEXT("index"));
	
		index = getIndex;

		State702();
	}
	else if(stateString == "704")
	{
		State704();
	}
	else if(stateString == "708")
	{
		PauseAndTimerReset();
	}
	// 성향 분석 결과
	else if(stateString == "709")
	{
		PauseAndTimerReset();
		
		// state 저장
		F_StateString = stateString;
		
		FString ID = object->GetStringField(TEXT("person_id"));

		// 정해진 state 오디오 찾아서 변수 저장
		if(DH_GM_state == EDHGameModeState::GM_KIM)
		{
			// 김구 일때
			FString back = "_kim.wav";

			FString final = ID.Append(back);
			
			UE_LOG(LogTemp, Warning, TEXT("audio dir : %s"), *final);
			
			MBTIAudio = final;
		}
		else if(DH_GM_state == EDHGameModeState::GM_AN)
		{
			// 안중근 일때
			FString back = "_an.wav";

			FString final = ID.Append(back);
			
			UE_LOG(LogTemp, Warning, TEXT("audio dir : %s"), *final);
			
			MBTIAudio = final;
		}
		else if(DH_GM_state == EDHGameModeState::GM_YUN)
		{
			// 윤봉길 일때
			FString back = "_yun.wav";

			FString final = ID.Append(back);
			
			UE_LOG(LogTemp, Warning, TEXT("audio dir : %s"), *final);
			
			MBTIAudio = final;
		}
		
		// mbti 오디오 재생
		SetTrackMBTI();
	}
	// 퀴즈
	else if(stateString == "901")
	{
		// state 저장
		F_StateString = stateString;

		State901();
	}
	else if(stateString == "903")
	{
		int32 q_index_String = object->GetNumberField(TEXT("q_index"));

		index = q_index_String;
		
		// 파람 저장
		F_StateString = stateString;
		F_TypeString = type_String;
		
		State903();
	}
	else if(stateString == "909")
	{
		// state 저장
		F_StateString = stateString;

		State909();
	}
	else if(stateString == "910")
	{
		// state 저장
		F_StateString = stateString;

		State910();
	}
	else if(stateString == "911")
	{
		// state 저장
		F_StateString = stateString;

		State911();
	}
	else if(stateString == "913")
	{
		// state 저장
		F_StateString = stateString;

		State913();
	}
	else if(stateString == "916")
	{
		// state 저장
		F_StateString = stateString;

		int32 index_String = object->GetNumberField(TEXT("n_correct"));
		
		State916(index_String);
	}
	else if(stateString == "918")
	{
		// state 저장
		F_StateString = stateString;

		State918();
	}
	else if(stateString == "920")
	{
		State920();
	}
	else if(stateString == "924")
	{
		// state 저장
		F_StateString = stateString;

		State924();
	}
	// 인물 변경
	else if(stateString == "1000")
	{
		// state 저장
		F_StateString = stateString;

		State1000();
	}
	else if(stateString == "1005")
	{
		PauseAndTimerReset();
		
		// state 저장
		F_StateString = stateString;
		
		// 인물 변경 및 등장
		FString DH_ID = object->GetStringField(TEXT("dgt_hm_id"));
		
		if(DH_ID == "H0004")
		{
			// 김구
			if(DH_GM_state == EDHGameModeState::GM_AN)
			{
				// 안중근에서 김구로
				GoAnToKim();
			}
			else
			{
				// 윤봉길에서 김구로
				GoYunToKim();
			}
		}
		else if(DH_ID == "H0005")
		{
			// 안중근
			if(DH_GM_state == EDHGameModeState::GM_KIM)
			{
				// 김구에서 안중근으로
				GoKimToAn();
			}
			else
			{
				// 윤봉길에서 안중근으로
				GoYunToAn();
			}
		}
		else if(DH_ID == "H0006")
		{
			// 윤봉길
			if(DH_GM_state == EDHGameModeState::GM_KIM)
			{
				// 김구에서 윤봉길로
				GoKimToYun();
			}
			else
			{
				// 안중근에서 윤봉길로
				GoAnToYun();
			}
		}
	}
	else if(stateString == "1007")
	{
		PauseAndTimerReset();

		CheckTwo();
		
		// 파람 저장
		F_StateString = stateString;
		F_TypeString = type_String;
		F_FileString = filename_String;
		F_GestureString = gesture_String;
		
		// 오디오 재생
		SetTrack();
	}
	// 도움말
	else if(stateString == "1100")
	{
		// state 저장
		F_StateString = stateString;

		State1100();
	}
}

void ADokGameModeBase::State0()
{
	if(F_StateString == "0")
	{
		State0Timer();
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("not state 0"));
	}

	EndError();
}

void ADokGameModeBase::State0Timer()
{
	F_GestureString = TEXT("뒷짐");

	SetGesture();

	FTimerHandle timer;

	GetWorldTimerManager().SetTimer(timer, this, &ADokGameModeBase::State0, 20, false);
}

void ADokGameModeBase::State1()
{
	// 오디오 정지
	GetTime();

	EndError();
}

void ADokGameModeBase::State9()
{
	PauseAndTimerReset();
	
	OnError();
}

void ADokGameModeBase::State300()
{
	F_GestureString = TEXT("경청");

	SetGesture();
}

void ADokGameModeBase::State601()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		OnceAudio = "kim601.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		OnceAudio = "an601.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		OnceAudio = "yun601.wav";
	}

	// once 오디오 재생
	SetTrackOnce();
}

void ADokGameModeBase::State602()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		OnceAudio = "kim602.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		OnceAudio = "an602.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		OnceAudio = "yun602.wav";
	}

	// 카메라 시작
	StartCamOn();
	
	// once 오디오 재생
	SetTrackOnce();
}

void ADokGameModeBase::State603()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		OnceAudio = "kim603.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		OnceAudio = "an603.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		OnceAudio = "yun603.wav";
	}

	// once 오디오 재생
	SetTrackOnce();
}

void ADokGameModeBase::State607()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		OnceAudio = "kim607.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		OnceAudio = "an607.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		OnceAudio = "yun607.wav";
	}
			
	// once 오디오 재생
	SetTrackOnce();
}

void ADokGameModeBase::State702()
{
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		FString front = "H0004_P_";

		FString num = FString::FromInt(index);
		FString back = ".wav";
				
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
			
		UE_LOG(LogTemp, Warning, TEXT("audio dir : %s"), *final);
		
		MBTIAudio = final;
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		FString front = "H0005_P_";

		FString num = FString::FromInt(index);
		FString back = ".wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
		
		UE_LOG(LogTemp, Warning, TEXT("audio dir : %s"), *final);
		
		MBTIAudio = final;
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		FString front = "H0006_P_";

		FString num = FString::FromInt(index);
		FString back = ".wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
		
		UE_LOG(LogTemp, Warning, TEXT("audio dir : %s"), *final);
		
		MBTIAudio = final;
	}

	bool random = FMath::RandBool();

	if(random)
	{
		// 제스쳐 추가하기
		F_GestureString = TEXT("한팔");
	}
	else
	{
		// 제스쳐 추가하기
		F_GestureString = TEXT("한팔_강조");
	}

	SetGesture();
	
	// mbti 오디오 재생
	SetTrackMBTI();
}

void ADokGameModeBase::State704()
{
	F_GestureString = TEXT("경청");

	SetGesture();
}

void ADokGameModeBase::State901()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		QuizAudio = "kim901.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		QuizAudio = "an901.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		QuizAudio = "yun901.wav";
	} 
	
	// once 오디오 재생
	SetTrackQuiz();
}

void ADokGameModeBase::State903()
{
	PauseAndTimerReset();
	
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		FString front = "H0004_q_";

		FString num = FString::FromInt(index);
		FString back = ".wav";
				
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
			
		UE_LOG(LogTemp, Warning, TEXT("audio dir : %s"), *final);
		
		F_FileString = final;
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		FString front = "H0005_q_";

		FString num = FString::FromInt(index);
		FString back = ".wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
		
		UE_LOG(LogTemp, Warning, TEXT("audio dir : %s"), *final);
		
		F_FileString = final;
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		FString front = "H0006_q_";

		FString num = FString::FromInt(index);
		FString back = ".wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
		
		UE_LOG(LogTemp, Warning, TEXT("audio dir : %s"), *final);
		
		F_FileString = final;
	}

	bool random = FMath::RandBool();

	if(random)
	{
		// 제스쳐 추가하기
		F_GestureString = TEXT("한팔");
	}
	else
	{
		// 제스쳐 추가하기
		F_GestureString = TEXT("한팔_강조");
	}
	
	// 오디오 재생
	SetTrack();
}

void ADokGameModeBase::State909()
{
	PauseAndTimerReset();
	
	int32 random = FMath::RandRange(1,4);

	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		FString front = "ans";
		FString num = FString::FromInt(random);
		FString back = "_kim.wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
				
		QuizAudio = final;
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		FString front = "ans";
		FString num = FString::FromInt(random);
		FString back = "_an.wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
				
		QuizAudio = final;
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		FString front = "ans";
		FString num = FString::FromInt(random);
		FString back = "_yun.wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
				
		QuizAudio = final;
	}

	// once 오디오 재생
	SetTrackQuiz();
}

void ADokGameModeBase::State910()
{
	PauseAndTimerReset();
	
	int32 random = FMath::RandRange(1,2);

	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		FString front = "wrong";
		FString num = FString::FromInt(random);
		FString back = "_kim.wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
				
		QuizAudio = final;
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		FString front = "wrong";
		FString num = FString::FromInt(random);
		FString back = "_an.wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
				
		QuizAudio = final;
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		FString front = "wrong";
		FString num = FString::FromInt(random);
		FString back = "_yun.wav";
		
		FString endFront = front.Append(num);
		FString final = endFront.Append(back);
				
		QuizAudio = final;
	}
	
	// once 오디오 재생
	SetTrackQuiz();
}

void ADokGameModeBase::State911()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		QuizAudio = "time_kim.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		QuizAudio = "time_an.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		QuizAudio = "time_yun.wav";
	}
	
	// once 오디오 재생
	SetTrackQuiz();
}

void ADokGameModeBase::State913()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		QuizAudio = "end_kim.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		QuizAudio = "end_an.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		QuizAudio = "end_yun.wav";
	}

	// once 오디오 재생
	SetTrackQuiz();
}

void ADokGameModeBase::State916(int32 idx)
{
	PauseAndTimerReset();

	if(idx == 0)
	{
		// 정해진 state 오디오 찾아서 변수 저장
		if(DH_GM_state == EDHGameModeState::GM_KIM)
		{
			// 김구 일때
			QuizAudio = "result_kim_0.wav";
		}
		else if(DH_GM_state == EDHGameModeState::GM_AN)
		{
			// 안중근 일때
			QuizAudio = "result_an_0.wav";
		}
		else if(DH_GM_state == EDHGameModeState::GM_YUN)
		{
			// 윤봉길 일때
			QuizAudio = "result_yun_0.wav";
		}
	}
	else if(idx == 1 || idx == 2)
	{
		// 정해진 state 오디오 찾아서 변수 저장
		if(DH_GM_state == EDHGameModeState::GM_KIM)
		{
			// 김구 일때
			QuizAudio = "result_kim_1.wav";
		}
		else if(DH_GM_state == EDHGameModeState::GM_AN)
		{
			// 안중근 일때
			QuizAudio = "result_an_1.wav";
		}
		else if(DH_GM_state == EDHGameModeState::GM_YUN)
		{
			// 윤봉길 일때
			QuizAudio = "result_yun_1.wav";
		}
	}
	else
	{
		// 정해진 state 오디오 찾아서 변수 저장
		if(DH_GM_state == EDHGameModeState::GM_KIM)
		{
			// 김구 일때
			QuizAudio = "result_kim_3.wav";
		}
		else if(DH_GM_state == EDHGameModeState::GM_AN)
		{
			// 안중근 일때
			QuizAudio = "result_an_1.wav";
		}
		else if(DH_GM_state == EDHGameModeState::GM_YUN)
		{
			// 윤봉길 일때
			QuizAudio = "result_yun_3.wav";
		}
	}
	
	// once 오디오 재생
	SetTrackQuiz();
}

void ADokGameModeBase::State918()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		QuizAudio = "name_kim.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		QuizAudio = "name_an.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		QuizAudio = "name_yun.wav";
	}
	
	// once 오디오 재생
	SetTrackQuiz();
}

void ADokGameModeBase::State920()
{
	F_GestureString = TEXT("경청");

	SetGesture();
}

void ADokGameModeBase::State924()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		QuizAudio = "rank_kim.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		QuizAudio = "rank_an.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		QuizAudio = "rank_yun.wav";
	}

	// once 오디오 재생
	SetTrackQuiz();
}

void ADokGameModeBase::State1000()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		OnceAudio = "kim1000.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		OnceAudio = "an1000.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		OnceAudio = "yun1000.wav";
	}

	// once 오디오 재생
	SetTrackOnce();
}

void ADokGameModeBase::State1100()
{
	PauseAndTimerReset();
	
	// 정해진 state 오디오 찾아서 변수 저장
	if(DH_GM_state == EDHGameModeState::GM_KIM)
	{
		// 김구 일때
		OnceAudio = "kim1100.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_AN)
	{
		// 안중근 일때
		OnceAudio = "an1100.wav";
	}
	else if(DH_GM_state == EDHGameModeState::GM_YUN)
	{
		// 윤봉길 일때
		OnceAudio = "yun1100.wav";
	}
			
	// once 오디오 재생
	SetTrackOnce();
}

void ADokGameModeBase::SetAnswer()
{
	// 오디오 파일명 없을때
	if(F_FileString == "")
	{
		// 바로 콜백
		CallBackMQTT();
	}
	// 파일명 있을때
	else if (F_FileString != "")
	{
		// 정해진 audio 재생
		SetTrack();
	}
}

void ADokGameModeBase::SetNotAnswer()
{
	if(F_FileString == "")
	{
		PlayNotAnswerGesture();
	}
	else
	{
		SetTrack();

		bCheckNotAnswer = true;
	}
}

void ADokGameModeBase::DH_GMP_CHANGE()
{
	switch (DH_GMP_state)
	{
	case EDHGameModePlayState::GMP_SIT:
		CheckGm_Sit();
		break;
	case EDHGameModePlayState::GMP_MOVEFRONT:
		CheckGM_MoveFront();
		break;
	case EDHGameModePlayState::GMP_STAND:
		CheckGM_Stand();
		break;
	case EDHGameModePlayState::GMP_MOVEBACK:
		CheckGM_MoveBack();
		break;
	}
}

void ADokGameModeBase::SetDH_GMP_State(EDHGameModePlayState next)
{
	DH_GMP_state = next;

	DH_GMP_CHANGE();
}

void ADokGameModeBase::CheckGM_MoveFront()
{
	StartSequencePlay();
}

void ADokGameModeBase::CheckGM_MoveBack()
{
	EndSequencePlay();
}

// 들어온 mqtt 있는지 체크 및 상태 변환
void ADokGameModeBase::InputReady()
{
	if(bInputActivate != false && DH_GMP_state == EDHGameModePlayState::GMP_SIT)
	{
		SetDH_GMP_State(EDHGameModePlayState::GMP_MOVEFRONT);

		bInputActivate = false;

		bInputDeactivate = false;
	}
	else if(bInputDeactivate != false && DH_GMP_state == EDHGameModePlayState::GMP_STAND)
	{
		SetDH_GMP_State(EDHGameModePlayState::GMP_MOVEBACK);

		bInputActivate = false;
		
		bInputDeactivate = false;
	}
	else
	{
		bInputActivate = false;

		bInputDeactivate = false;
	}
}

void ADokGameModeBase::PlayAudioSetTrack()
{
	SetTrack();

	UE_LOG(LogTemp, Warning, TEXT("인사말 오디오"));
}

// -------------------------------------------------------------------------------input
// 앞으로 가라는 입력
void ADokGameModeBase::InputMoveFront()
{
	// 시퀀스가 실행중이 아니고 뒤에 있는 상태 일 때  
	if(DH_GMP_state == EDHGameModePlayState::GMP_SIT)
	{
		// 앞으로 상태 변환
		SetDH_GMP_State(EDHGameModePlayState::GMP_MOVEFRONT);
	}
	// 시퀀스가 실행 중 일 때 
	else
	{
		bInputActivate = true;
	}
}

// 뒤로 가라는 입력
void ADokGameModeBase::InputMoveBack()
{
	// 시퀀스가 실행중이 아니고 앞에 있는 상태 일 때
	if(DH_GMP_state == EDHGameModePlayState::GMP_STAND)
	{
		// 뒤로 상태 변환
		SetDH_GMP_State(EDHGameModePlayState::GMP_MOVEBACK);
	}
	// 시퀀스가 실행 중 일 때
	else
	{
		bInputDeactivate = true;
	}
}

void ADokGameModeBase::SetTrackMBTI()
{
	// audio track setting
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");
	RequestObj->SetStringField("file_name", MBTIAudio);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseSetTrackMBTI);
	Request->SetURL("http://localhost:8011/A2F/Player/SetTrack");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseSetTrackMBTI(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseSetTrack %s"), *Response->GetContentAsString());

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		GetRangeMBTI();
	}
}

void ADokGameModeBase::GetRangeMBTI()
{
	// get audio range
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");
	
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseGetRangeMBTI);
	Request->SetURL("http://localhost:8011/A2F/Player/GetRange");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseGetRangeMBTI(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseGetRange %s"), *Response->GetContentAsString());

	// json 구조체 읽어서 오디오 길이 float에 담기
	TSharedPtr<FJsonObject> object = ResponseObj->GetObjectField(TEXT("result"));
	TArray<TSharedPtr<FJsonValue>> objectArray = object->GetArrayField(TEXT("default"));

	if(objectArray[1] != nullptr)
	{
		auto idx = objectArray[1]->AsNumber();

		audioRange = Chaos::ConvertDoubleToFloat(idx);
	}
	else
	{
		audioRange = 0;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ResponseGetRange %f"), audioRange);

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		PlayMBTI();
	}
}

void ADokGameModeBase::PlayMBTI()
{
	// post audio play
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponsePlayMBTI);
	Request->SetURL("http://localhost:8011/A2F/Player/Play");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponsePlayMBTI(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponsePlay %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{
		if(F_StateString != "709")
		{
			// audio 길이만큼 시간이 흐른 다음
			//GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackMBTI, audioRange, false);

			//F_GestureString = TEXT("인식");
			
			//SetGesture();
		}
	}
}


void ADokGameModeBase::SetTrackQuiz()
{
	// audio track setting
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");
	RequestObj->SetStringField("file_name", QuizAudio);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseSetTrackQuiz);
	Request->SetURL("http://localhost:8011/A2F/Player/SetTrack");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseSetTrackQuiz(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseSetTrack %s"), *Response->GetContentAsString());

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		GetRangeQuiz();
	}
}

void ADokGameModeBase::GetRangeQuiz()
{
	// get audio range
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");
	
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseGetRangeQuiz);
	Request->SetURL("http://localhost:8011/A2F/Player/GetRange");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseGetRangeQuiz(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseGetRange %s"), *Response->GetContentAsString());

	// json 구조체 읽어서 오디오 길이 float에 담기
	TSharedPtr<FJsonObject> object = ResponseObj->GetObjectField(TEXT("result"));
	TArray<TSharedPtr<FJsonValue>> objectArray = object->GetArrayField(TEXT("default"));

	if(objectArray[1] != nullptr)
	{
		auto idx = objectArray[1]->AsNumber();

		audioRange = Chaos::ConvertDoubleToFloat(idx);
	}
	else
	{
		audioRange = 0;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ResponseGetRange %f"), audioRange);

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		PlayQuiz();
	}
}

void ADokGameModeBase::PlayQuiz()
{
	// post audio play
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponsePlayQuiz);
	Request->SetURL("http://localhost:8011/A2F/Player/Play");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponsePlayQuiz(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponsePlay %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{
		if(F_StateString == "901")
		{
			// audio 길이만큼 시간이 흐른 다음
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackQuiz, audioRange, false);

			F_GestureString = TEXT("한팔");
			
			SetGesture();
		}
		else if(F_StateString == "909")
		{
			// audio 길이만큼 시간이 흐른 다음
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackQuiz, audioRange, false);

			F_GestureString = TEXT("양팔");
			
			SetGesture();
		}
		else if(F_StateString == "910" || F_StateString == "911")
		{
			// audio 길이만큼 시간이 흐른 다음
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackQuiz, audioRange, false);

			F_GestureString = TEXT("불용");
			
			SetGesture();
		}
		else if(F_StateString == "913")
		{
			// audio 길이만큼 시간이 흐른 다음
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackQuiz, audioRange, false);
		}
		else if(F_StateString == "916")
		{
			// audio 길이만큼 시간이 흐른 다음
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackQuiz, audioRange, false);

			F_GestureString = TEXT("양팔_강조");
			
			SetGesture();
		}
		else if(F_StateString == "918")
		{
			// audio 길이만큼 시간이 흐른 다음
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackQuiz, audioRange, false);
		}
		else if(F_StateString == "924")
		{
			// audio 길이만큼 시간이 흐른 다음
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackQuiz, audioRange, false);

			F_GestureString = TEXT("인식");
			
			SetGesture();
		}
	}
}

void ADokGameModeBase::SetTrackOnce()
{
	// audio track setting
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");
	RequestObj->SetStringField("file_name", OnceAudio);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseSetTrackOnce);
	Request->SetURL("http://localhost:8011/A2F/Player/SetTrack");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseSetTrackOnce(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseSetTrack %s"), *Response->GetContentAsString());

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		GetRangeOnce();
	}
}

void ADokGameModeBase::GetRangeOnce()
{
	// get audio range
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");
	
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponseGetRangeOnce);
	Request->SetURL("http://localhost:8011/A2F/Player/GetRange");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponseGetRangeOnce(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponseGetRange %s"), *Response->GetContentAsString());

	// json 구조체 읽어서 오디오 길이 float에 담기
	TSharedPtr<FJsonObject> object = ResponseObj->GetObjectField(TEXT("result"));
	TArray<TSharedPtr<FJsonValue>> objectArray = object->GetArrayField(TEXT("default"));

	if(objectArray[1] != nullptr)
	{
		auto idx = objectArray[1]->AsNumber();

		audioRange = Chaos::ConvertDoubleToFloat(idx);
	}
	else
	{
		audioRange = 0;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ResponseGetRange %f"), audioRange);

	if(bConnectedSuccessfully != false)
	{
		// 끝나면 다음 과정으로
		PlayOnce();
	}
}

void ADokGameModeBase::PlayOnce()
{
	// post audio play
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("a2f_player", "/World/audio2face/Player");

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);

	Request->OnProcessRequestComplete().BindUObject(this, &ADokGameModeBase::ResponsePlayOnce);
	Request->SetURL("http://localhost:8011/A2F/Player/Play");
	Request->SetVerb("Post");
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

void ADokGameModeBase::ResponsePlayOnce(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	UE_LOG(LogTemp, Warning, TEXT("ResponsePlayOnce %s"), *Response->GetContentAsString());
	
	if(bConnectedSuccessfully != false)
	{

		UE_LOG(LogTemp, Warning, TEXT("play state is %s"), *F_StateString);
		
		// state 601, 1100 일때
		if(F_StateString == "601" || F_StateString == "1100")
		{
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackOnce, audioRange, false);

			F_GestureString = TEXT("한팔");
			
			SetGesture();
		}
		// state 602 일때
		else if(F_StateString == "602")
		{
			// 아무것도 안하기
		}
		// state 603 일때
		else if(F_StateString == "603")
		{
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackOnce, audioRange, false);
		}
		// state 605 사진 결과
		else if(F_StateString == "606")
		{
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackOnce, audioRange, false);

			F_GestureString = TEXT("양팔");
			
			SetGesture();
		}
		else if(F_StateString == "1000" || F_StateString == "1007")
		{
			GetWorldTimerManager().SetTimer(AudioTimer, this, &ADokGameModeBase::CallBackOnce, audioRange, false);
		}
	}
}