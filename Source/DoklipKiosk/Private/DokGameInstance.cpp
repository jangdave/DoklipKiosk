// Fill out your copyright notice in the Description page of Project Settings.


#include "DokGameInstance.h"
#include "DokGameModeBase.h"
#include "Common/TcpSocketBuilder.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetStringLibrary.h"
#include "Serialization/ArrayWriter.h"

void UDokGameInstance::Init()
{
	Super::Init();
	
	GM = Cast<ADokGameModeBase>(GetWorld()->GetAuthGameMode());

	// try
	// {
	// }
	// catch (...)
	// {
	// }
}

void UDokGameInstance::StartSocket()
{
	if(bCheckSocket != true)
	{
		// 9005 소켓 연결
		Listen9005();

		// 9004 소켓 연결
		FTimerHandle timer;
		GetTimerManager().SetTimer(timer, this, &UDokGameInstance::Connect9004, 1.0f, true);

		bCheckSocket = true;
	}
}

bool UDokGameInstance::Send(FSocket* Socket, const uint8* Buffer, int32 Size)
{
	while (Size > 0)
	{
		int32 BytesSent = 0;
		if (!Socket->Send(Buffer, Size, BytesSent))
		{
			return false;
		}

		Size -= BytesSent;
		Buffer += BytesSent;
	}

	return true;
}

bool UDokGameInstance::SendPacket(FSocket* Socket, uint32 Type, const TArray<uint8>& Payload)
{
	return SendPacket(Socket, Type, Payload.GetData(), Payload.Num());
}

bool UDokGameInstance::SendPacket(FSocket* Socket, uint32 Type, const uint8* Payload, int32 PayloadSize)
{
	FBufferArchive Ar;

	// append the payload bytes to send it in one network packet
	Ar.Append(Payload, PayloadSize);

	// Send it, and make sure it sent it all
	if (!Send(Socket, Ar.GetData(), Ar.Num()))
	{
		UE_LOG(LogSockets, Error, TEXT("Unable To Send."));
		return false;
	}
	return true;
}

void UDokGameInstance::Send(uint32 Type, const FString& Text)
{
	FTCHARToUTF8 Convert(*Text);
	FArrayWriter WriterArray;
	WriterArray.Serialize((UTF8CHAR*)Convert.Get(), Convert.Length());

	if (SendPacket(ConnectionSocket, Type, WriterArray))
	{
		//UE_LOG(LogTemp, Log, TEXT("Sent Text : %s  Size : %d"), *Text, WriterArray.Num());
	}
}

void UDokGameInstance::PrintSocketError(FString& OutText)
{
	ESocketErrors Err = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
	const TCHAR* SocketErr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError(Err);
	OutText = SocketErr;
}

void UDokGameInstance::Listen9005()
{
	if(StartTCPReceiver("ListenSocket", "127.0.0.1", 9005))
	{
		//UE_LOG(LogTemp, Warning, TEXT("TCP Socket Listener Created!"));
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("TCP Socket Listener Failed!"));
	}
}

bool UDokGameInstance::StartTCPReceiver(const FString SocketName, const FString TheIP, const int32 ThePort)
{
	ListenerSocket = CreateTCPConnectionListener(SocketName, TheIP, ThePort);

	if(!ListenerSocket)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Listen socket could not be created!"));
		
		return false;
	}

	FTimerHandle timer;
	GetTimerManager().SetTimer(timer, this, &UDokGameInstance::TCPConnectionListener, 1.0f, true);
	
	return true;
}

FSocket* UDokGameInstance::CreateTCPConnectionListener(const FString SocketName, const FString TheIP,
	const int32 ThePort, const int32 BufferSize)
{
	uint8 IP4Nums[4];

	if(!FormatIP4ToNumber(TheIP, IP4Nums))
	{
		return false;
	}

	FIPv4Endpoint Endpoint(FIPv4Address(IP4Nums[0], IP4Nums[1], IP4Nums[2], IP4Nums[3]), ThePort);
	FSocket* ListenSocket = FTcpSocketBuilder(*SocketName)
			.AsReusable()
			.BoundToEndpoint(Endpoint)
			.Listening(8);

	int32 Size = 0;
	ListenSocket->SetReceiveBufferSize(BufferSize, Size);
	
	return ListenSocket;
}

void UDokGameInstance::TCPConnectionListener()
{
	if(!ListenerSocket)
	{
		return;
	}

	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;

	if(ListenerSocket->HasPendingConnection(Pending) && Pending)
	{
		if (ConnectionSocket)
		{
			ConnectionSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket);
		}
		
		ConnectionSocket = ListenerSocket->Accept(*RemoteAddress, TEXT("Start Kick off Event"));

		if(ConnectionSocket != NULL)
		{
			RemoteAddressForConnection = FIPv4Endpoint(RemoteAddress);
			//UE_LOG(LogTemp, Warning, TEXT("Accepted Connection!"));
			
			Send(0, TEXT("0"));
		}
	}
}

bool UDokGameInstance::FormatIP4ToNumber(const FString& TheIP, uint8(& Out)[4])
{
	const FString IP = TheIP.Replace(TEXT(" "),TEXT(""));
	
	TArray<FString> Parts;
	
	IP.ParseIntoArray(Parts, TEXT("."), true);
	
	if (Parts.Num() != 4)
	{
		return false;
	}
 
	for (int32 i = 0; i < 4; ++i)
	{
		Out[i] = FCString::Atoi(*Parts[i]);
	}
	
	return true;
}


void UDokGameInstance::Connect9004()
{
	if(StartTCPReceiver9004("ConnectSocket", "127.0.0.1", 9004))
	{
		//UE_LOG(LogTemp, Warning, TEXT("TCP Socket Connect Created!"));
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("TCP Socket Connect Failed!"));

		// fsm 상태 함수
		KioskFsmCheck(-1);
	}
}

bool UDokGameInstance::StartTCPReceiver9004(const FString SocketName, const FString TheIP, const int32 ThePort)
{
	ListenerSocket9004 = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, SocketName, false);
	
	FIPv4Address ip;
	FIPv4Address::Parse(TheIP, ip);

	TSharedRef<FInternetAddr> internetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	internetAddr->SetIp(ip.Value);
	internetAddr->SetPort(ThePort);
	
	if (!ListenerSocket9004)
	{
		return false;
	}
	
	bool bConnect = ListenerSocket9004->Connect(*internetAddr);

	if(!bConnect)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Connect socket fail!"));
		
		FString ErrorText;
		PrintSocketError(ErrorText);
		
		UE_LOG(LogTemp, Error, TEXT("Socket Connect Failed.  Error : %s  ConnectionState : %d"),*ErrorText, (int32)ListenerSocket9004->GetConnectionState());

		return false;
	}
	
	//UE_LOG(LogTemp, Warning, TEXT("Connect socket sucess!"));

	uint32 bufferSize = 1024;
	uint8 buffer[1024];
	int32 size = 0;

	if(ListenerSocket9004->Recv(buffer, bufferSize, size))
	{
		FString ReceivedData = UTF8_TO_TCHAR(reinterpret_cast<const char*>(buffer));
		
		//UE_LOG(LogTemp, Warning, TEXT("Received data: %s"), *ReceivedData);

		int32 state = FCString::Atoi(*ReceivedData);
		
		//UE_LOG(LogTemp, Warning, TEXT("Received data int: %d"), state);

		// fsm 상태 함수
		KioskFsmCheck(state);
		
		ListenerSocket9004->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket9004);
	}

	return true;
}

void UDokGameInstance::KioskFsmCheck(int32 state)
{
	// state 체크 함수 실행
	if(!StateCheck(state))
	{
		// -1 -> -1
		// 0 -> 0
		// 현 상태 저장
		fsmState = state;
	}
	else
	{
		// 0 -> -1
		if(state == -1)
		{
			// 오디오 끄기 실행
			GM->Pause();
			
			// 현 상태 저장
			fsmState = state;
		}
		// -1 -> 0
		else if(state == 0)
		{
			if(bCheckRestart == false)
			{
				// 현 상태 저장
				fsmState = state;

				bCheckRestart = true;
			}
			else
			{
				// 레벨 재시작
				ReStartLevel();

				// 현 상태 저장
				fsmState = state;
			}
		}
	}
}

bool UDokGameInstance::StateCheck(int32 state)
{
	// 첫 실행일 때
	if(bCheckOn != true)
	{
		bCheckOn = true;

		
		return false;
	}
	
	// 그 다음 실행
	// 전 상태랑 같다면
	if(fsmState == state)
	{
		return false;
	}
	
	return true;
}

void UDokGameInstance::ReStartLevel()
{
	FString name = GetWorld()->GetName();

	FName levelName = UKismetStringLibrary::Conv_StringToName(name);

	UE_LOG(LogTemp, Warning, TEXT("name : %s"), *levelName.ToString());
				
	UGameplayStatics::OpenLevel(GetWorld(), levelName);

	UE_LOG(LogTemp, Warning, TEXT("Level Restart!!"));
}

void UDokGameInstance::InputTrue()
{
	bCheckInput0 = true;
}

void UDokGameInstance::InputFalse()
{
	bCheckInput0 = false;
}
