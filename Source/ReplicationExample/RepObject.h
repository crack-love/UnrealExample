#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Net/UnrealNetwork.h"
#include "RepObject.generated.h"

UCLASS()
class URepObject : public UObject
{
	GENERATED_BODY()

public:
	URepObject() 
	{

	}

	bool IsSupportedForNetworking() const override { return true; }

	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

		DOREPLIFETIME(URepObject, Value);
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

	float GetValue() { return Value; }

	UFUNCTION()
	void OnRep_Value()
	{
		UE_LOG(LogTemp, Log, TEXT("URepObject.OnRep_Object Value %f, IsServer:%d, IsClient:%d"), Value,
			GetWorld()->IsServer(), GetWorld()->IsClient());
	}

private:
	UPROPERTY(ReplicatedUsing = OnRep_Value)
	float Value;
};

