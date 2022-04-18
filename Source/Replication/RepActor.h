#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RepActor.generated.h"

USTRUCT(BlueprintType)
struct FRepStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float Value;
};

UCLASS()
class ARepActor : public AActor
{
	GENERATED_BODY()

public:
	ARepActor();

public:
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_MoveNext(int param);

	UFUNCTION(Client, Reliable)
	void Client_Notify();

protected:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool ReplicateSubobjects(class UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void OnRep_Value();

	// Called when struct's data is changed
	UFUNCTION()
	void OnRep_Struct();

	// Called when pointer is changed
	// if want to callback from uobject's data changed
	// bind replicatiedUsing to it's data, that will be called
	UFUNCTION()
	void OnRep_UObject();

private:
	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Value)
	float Value;

	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_UObject)
	class URepUObject* UObjectValue;

	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Struct)
	FRepStruct StructValue;

	class UStaticMeshComponent* MeshComponent;
};