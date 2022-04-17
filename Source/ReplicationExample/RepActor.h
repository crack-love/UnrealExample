#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/ActorChannel.h"

#include "RepObject.h"
#include "RepActor.generated.h"

UCLASS()
class ARepActor : public AActor
{
	GENERATED_BODY()

public:
	ARepActor()
	{
		PrimaryActorTick.bCanEverTick = true;
		SetReplicates(true);
		SetReplicateMovement(true);
		
		// Create MeshComponent
		MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
		static auto MeshFinder = ConstructorHelpers::FObjectFinder<UStaticMesh>(
			TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
		if (MeshFinder.Succeeded())
		{
			MeshComponent->SetStaticMesh(MeshFinder.Object);
		}
		SetRootComponent(MeshComponent);

		if (HasAuthority()) 
		{
			// Objects only replicate from server to client. If we didn't guard this
			// the client would create the object just fine but it would get replaced 
			// by the server version (more accurately the property would be replaced to 
			// point to the version from the server. The one the client allocated would 
			// eventually be garbage collected.
			
			// NOTE: Very important, objects Outer must be our Actor! 
			ObjectValue = CreateDefaultSubobject<URepObject>(TEXT("SubObject"));
		}
	}

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

		DOREPLIFETIME(ARepActor, Value);
		DOREPLIFETIME(ARepActor, ObjectValue);

		// Add any Blueprint properties
		// This is not required if you do not want the class to be "Blueprintable"
		if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
		{
			BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
		}
	}

	bool ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override
	{
		bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

		WroteSomething |= Channel->ReplicateSubobject(ObjectValue, *Bunch, *RepFlags);

		return WroteSomething;
	}

	void Tick(float DeltaSeconds) override
	{
		Super::Tick(DeltaSeconds);

		if (HasAuthority())
		{
			Server_MoveNext(1);
		}
	}

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_MoveNext(int param);
	void Server_MoveNext_Implementation(int param)
	{
		check(HasAuthority());

		Value += param;
		AddActorLocalOffset(FVector::OneVector * 0.1f * param);

		ObjectValue->Server_SetValue(Value);
	}
	bool Server_MoveNext_Validate(int param)
	{
		return true;
	}

	UFUNCTION()
	void OnRep_Value()
	{
		UE_LOG(LogTemp, Log, TEXT("ARepActor.OnRep_Value %f, IsServer:%d, IsClient:%d"), Value, 
			GetWorld()->IsServer(), GetWorld()->IsClient());
	}

	UFUNCTION()
	void OnRep_Object()
	{
		UE_LOG(LogTemp, Log, TEXT("ARepActor.OnRep_Object Value %f, IsServer:%d, IsClient:%d"), ObjectValue->GetValue(),
			GetWorld()->IsServer(), GetWorld()->IsClient());
	}

private:
	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Value)
	float Value;

	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Object)
	URepObject* ObjectValue;

	UStaticMeshComponent* MeshComponent;
};