#pragma once

#include "CoreMinimal.h"

// Format text 
// TEXTF : return FString
// TEXTFP : return const TCHAR*
#define TEXTF(MSG, ...) FString::Printf(TEXT(MSG),##__VA_ARGS__)
#define TEXTFP(MSG, ...) *TEXTF(MSG,##__VA_ARGS__)

class FLog
{
public:
	static void Log(const FString& Message)
	{
		Log(*Message);
	}

	static void Log(const TCHAR* Message)
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), Message);
	}

	static void Screen(const FString& Message, int32 Key = -1, float Sec = 2.f)
	{
		GEngine->AddOnScreenDebugMessage(Key, Sec, FColor::Green, Message);
	}
};