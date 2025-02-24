// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "DokGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class DOKLIPKIOSK_API UDokGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	void StartSocket();

	static bool Send(FSocket* Socket, const uint8* Buffer, int32 Size);
	
	static bool SendPacket(FSocket* Socket, uint32 Type, const TArray<uint8>& Payload);
	
	static bool SendPacket(FSocket* Socket, uint32 Type, const uint8* Payload, int32 PayloadSize);
	
	void Send(uint32 Type, const FString& Text);

	static void PrintSocketError(FString& OutText);

	//---------------------------------------------------------------------------------
	void Listen9005();
	
	FSocket* ListenerSocket;
	
	FSocket* ConnectionSocket;
	
	FIPv4Endpoint RemoteAddressForConnection;

	bool StartTCPReceiver(const FString SocketName, const FString TheIP, const int32 ThePort);

	FSocket* CreateTCPConnectionListener(const FString SocketName, const FString TheIP, const int32 ThePort, const int32 BufferSize = 2*1024*1024);
	
	void TCPConnectionListener();
	
	bool FormatIP4ToNumber(const FString& TheIP, uint8 (&Out)[4]);

	//----------------------------------------------------------------------------------
	void Connect9004();
	
	FSocket* ListenerSocket9004;

	FSocket* ConnectionSocket9004;

	bool StartTCPReceiver9004(const FString SocketName, const FString TheIP, const int32 ThePort);

	void KioskFsmCheck(int32 state);
	
	bool StateCheck(int32 state);
	
	void ReStartLevel();

	UPROPERTY()
	bool bCheckOn = false;

	UPROPERTY()
	bool bCheckSocket = false;

	UPROPERTY()
	bool bCheckRestart = false;
	
	UPROPERTY()
	int32 fsmState;
	
	UPROPERTY()
	class ADokGameModeBase* GM;

	UPROPERTY()
	bool bCheckInput0 = false;

	UFUNCTION()
	void InputTrue();

	UFUNCTION()
	void InputFalse();
};
