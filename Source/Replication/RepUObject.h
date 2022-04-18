#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Net/UnrealNetwork.h"
#include "Log.h"
#include "RepUObject.generated.h"

UCLASS()
class URepUObject : public UObject
{
	GENERATED_BODY()

public:
	URepUObject() 
	{

	}

	bool IsSupportedForNetworking() const override { return true; }

	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

		DOREPLIFETIME(URepUObject, Value);
	}

	UFUNCTION()
	void SetValue(float NewValue)
	{
		Value = NewValue;
	}

	UFUNCTION(Server, Reliable)
	void Server_SetValue(float NewValue);
	void Server_SetValue_Implementation(float NewValue)
	{
		if (GetWorld()->IsServer())
		{
			Value = NewValue;
		}
	}

	UFUNCTION()
	void OnRep_Value()
	{
		FLog::Log(TEXTF("URepObject.OnRep_Value %f", Value));
	}

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Value)
	float Value;
};

