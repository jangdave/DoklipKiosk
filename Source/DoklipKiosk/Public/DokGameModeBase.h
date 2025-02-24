// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Interfaces/IHttpRequest.h"
#include "DokGameModeBase.generated.h"

UENUM(BlueprintType)
enum class EDHGameModeState : uint8
{
	// 인물 상태 저장 enum
	GM_KIM,
	GM_AN,
	GM_YUN,
};

UENUM(BlueprintType)
enum class EDHGameModePlayState : uint8
{
	// 인물 상태 저장 enum
	GMP_SIT,
	GMP_MOVEFRONT,
	GMP_STAND,
	GMP_MOVEBACK,
}; 

/**
 * 
 */

UCLASS()
class DOKLIPKIOSK_API ADokGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	ADokGameModeBase();

public:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	class UDokGameInstance* GI;

	// 소켓 연결 함수
	UFUNCTION(BlueprintCallable)
	void StartSocketOn();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EDHGameModeState DH_GM_state;
	
	// state 변경 함수
	void DH_GM_CHANGE();
	
	// state 변경 함수
	UFUNCTION(BlueprintCallable)
	void SetDH_GM_State(EDHGameModeState next);

	// 인물 변경 함수
	UFUNCTION(BlueprintImplementableEvent)
	void GoAnToKim();

	UFUNCTION(BlueprintImplementableEvent)
	void GoYunToKim();

	UFUNCTION(BlueprintImplementableEvent)
	void GoKimToAn();

	UFUNCTION(BlueprintImplementableEvent)
	void GoYunToAn();

	UFUNCTION(BlueprintImplementableEvent)
	void GoKimToYun();

	UFUNCTION(BlueprintImplementableEvent)
	void GoAnToYun();

	void GoKim();
	
	void GoAn();

	void GoYun();

	// audio2face 링크 연결/해제 함수
	UFUNCTION(BlueprintImplementableEvent)
	void SetKimARKitOn();

	UFUNCTION(BlueprintImplementableEvent)
	void SetKimARKitOff();
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetAnARKitOn();

	UFUNCTION(BlueprintImplementableEvent)
	void SetAnARKitOff();
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetYunARKitOn();

	UFUNCTION(BlueprintImplementableEvent)
	void SetYunARKitOff();
	
	// audio2face rest api 연결
	// 시작 세팅
	UFUNCTION(BlueprintCallable)
	void StartAudio2faceOn();
	
	void CheckStatus();

	void ResponseCheckStatus(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void CheckChangeStatus();

	void ResponseCheckChangeStatus(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void SetKimUSD();

	void ResponseSetKimUSD(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void SetAnUSD();

	void ResponseSetAnUSD(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void SetYunUSD();

	void ResponseSetYunUSD(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void SetActiveLiveLink();

	void ResponseSetActiveLiveLink(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	// 립싱크
	void SetTrack();

	void ResponseSetTrack(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void GetRange();

	void ResponseGetRange(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void Play();

	void ResponsePlay(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void GetTime();

	void ResponseGetTime(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void SetTrack0000();

	void ResponseSetTrack0000(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void Play0000();

	void ResponsePlay0000(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void Pause();

	void ResponsePause(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	UPROPERTY()
	FTimerHandle AudioTimer;

	UFUNCTION()
	void PauseAndTimerReset();
	
	// 제스쳐 세팅 함수
	UFUNCTION()
	void SetGesture();

	UPROPERTY()
	bool bCheckTwo = false;

	UFUNCTION(BlueprintCallable)
	void CheckTwo();
	
	// 제스쳐 플레이 함수
	UFUNCTION(BlueprintImplementableEvent)
	void PlayOnehand();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayTwohand();
	
	UFUNCTION(BlueprintImplementableEvent)
	void PlayChinrest();
	
	UFUNCTION(BlueprintImplementableEvent)
	void PlayArmacross();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayInsoluble();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayHello();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayCustom();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayBack();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayHear();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayRecognize();
	
	UFUNCTION(BlueprintImplementableEvent)
	void PlayOneCheck();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayTwoCheck();
	
	UFUNCTION(BlueprintImplementableEvent)
	void PlayNotAnswerGesture();
	
	// 정상 답변, 비정상 답변 체크 변수
	UPROPERTY()
	bool bCheckNotAnswer = false;
	
	// mqtt 통신
	// 오디오 길이 변수
	UPROPERTY()
	float audioRange = 0.0f;

	// answer, activate, deactivate 타입 저장 변수
	UPROPERTY()
	FString F_TypeString;

	// 재생 오디오 이름 저장 변수
	UPROPERTY()
	FString F_FileString;

	// 플레이 제스쳐 저장 변수
	UPROPERTY()
	FString F_GestureString;

	UPROPERTY(BlueprintReadOnly)
	FString F_StateString;

	// mqtt 통신 pub sub 함수
	UFUNCTION(BlueprintCallable)
	void CallReceivedMQTT(FString received);

	UFUNCTION()
	void State0();

	UFUNCTION()
	void State0Timer();
	
	UFUNCTION()
	void State1();

	UFUNCTION()
	void State9();

	UFUNCTION()
	void State300();
	
	UFUNCTION()
	void State601();

	UFUNCTION()
	void State602();

	UFUNCTION()
	void State603();

	UFUNCTION()
	void State607();

	UFUNCTION()
	void State702();
	
	UFUNCTION()
	void State704();

	UFUNCTION()
	void State901();

	UFUNCTION()
	void State903();

	UFUNCTION()
	void State909();

	UFUNCTION()
	void State910();
	
	UFUNCTION()
	void State911();

	UFUNCTION()
	void State913();

	UFUNCTION()
	void State916(int32 idx);

	UFUNCTION()
	void State918();

	UFUNCTION()
	void State920();

	UFUNCTION()
	void State924();

	UFUNCTION()
	void State1000();

	UFUNCTION()
	void State1100();
	
	// 정상 답변 함수
	void SetAnswer();

	// 비정상 답변 함수
	void SetNotAnswer();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void CallBackMQTT();

	UFUNCTION(BlueprintImplementableEvent)
	void EndFrontEndCallBackMQTT();

	UFUNCTION(BlueprintImplementableEvent)
	void EndBackEndCallBackMQTT();
	
	UFUNCTION(BlueprintImplementableEvent)
	void NotExistAudio();
	
	// GM state machine
	// gm state 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EDHGameModePlayState DH_GMP_state;
	
	// state 실행 함수
	void DH_GMP_CHANGE();

	// state 변경 함수
	UFUNCTION(BlueprintCallable)
	void SetDH_GMP_State(EDHGameModePlayState next);

	UFUNCTION(BlueprintImplementableEvent)
	void CheckGm_Sit();

	void CheckGM_MoveFront();

	UFUNCTION(BlueprintImplementableEvent)
	void CheckGM_Stand();
	
	void CheckGM_MoveBack();

	// 시퀀스
	// 시퀀스 변수
	UPROPERTY()
	bool bInputActivate = false;

	UPROPERTY()
	bool bInputDeactivate = false;

	// 시퀀스 함수
	UFUNCTION(BlueprintImplementableEvent)
	void StartSequencePlay();

	UFUNCTION(BlueprintImplementableEvent)
	void EndSequencePlay();

	UFUNCTION(BlueprintCallable)
	void InputReady();

	UFUNCTION(BlueprintCallable)
	void PlayAudioSetTrack();
	
	// 인물 변경
	UFUNCTION(BlueprintImplementableEvent)
	void CharacterSync();

	// 등장 퇴장
	UFUNCTION(BlueprintCallable)
	void InputMoveFront();

	UFUNCTION(BlueprintCallable)
	void InputMoveBack();

	// 성향 검사
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 index;

	UPROPERTY()
	FString MBTIAudio;
	
	void SetTrackMBTI();
	
	void ResponseSetTrackMBTI(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void GetRangeMBTI();
	
	void ResponseGetRangeMBTI(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void PlayMBTI();
	
	void ResponsePlayMBTI(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	UFUNCTION(BlueprintImplementableEvent)
	void CallBackMBTI();

	// 퀴즈
	UPROPERTY()
	FString QuizAudio;
	
	void SetTrackQuiz();
	
	void ResponseSetTrackQuiz(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void GetRangeQuiz();
	
	void ResponseGetRangeQuiz(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void PlayQuiz();
	
	void ResponsePlayQuiz(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	UFUNCTION(BlueprintImplementableEvent)
	void CallBackQuiz();
	
	// 카메라
	UFUNCTION(BlueprintImplementableEvent)
	void StartCamOn();

	UPROPERTY()
	FString OnceAudio;
	
	void SetTrackOnce();
	
	void ResponseSetTrackOnce(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void GetRangeOnce();
	
	void ResponseGetRangeOnce(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	void PlayOnce();
	
	void ResponsePlayOnce(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void CallBackOnce();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void CallBackError();

	UFUNCTION(BlueprintImplementableEvent)
	void OnError();

	UFUNCTION(BlueprintImplementableEvent)
	void EndError();
};
