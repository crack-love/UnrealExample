#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RepActor.generated.h"

// Declare Struct Type Data to know how replicate it
USTRUCT(BlueprintType)
struct FRepStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float Value;

	UPROPERTY(VisibleAnywhere)
	TArray<int32> ArrayValue;
};


UCLASS()
class ARepActor : public AActor
{
	GENERATED_BODY()

	// Ctors
public:
	ARepActor();

	void LazyInitialize();

	//
	// RPC Calls
	//
public:
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_Tick(int param);

	UFUNCTION(NetMulticast, Reliable)
	void Client_Notify();

	//
	// Engine APIs
	//
protected:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool ReplicateSubobjects(class UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	void Tick(float DeltaSeconds) override;

	//
	// Replicated Callbacks
	//
private:
	UFUNCTION()
	void OnRep_Value();

	// Called when struct's any data is changed
	UFUNCTION()
	void OnRep_Struct();

	// Called when pointer is changed
	// if want to callback from uobject's data changed
	// bind replicatiedUsing to it's data, that will be called
	UFUNCTION()
	void OnRep_UObject();

	// Called when Array Element is changed
	// Replicate only changed element not whole array blck
	UFUNCTION()
	void OnRep_Array();
	
	//
	// Private methods
	//
private:
	void LogArray(TCHAR* Tag, TArray<int32>& Array);

	//
	// Replicate Fields
	//
private:
	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Value)
	float Value;

	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Struct)
	FRepStruct StructValue;

	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_Array)
	TArray<int32> ArrayValue;

	UPROPERTY(Replicated, VisibleAnywhere, ReplicatedUsing = OnRep_UObject)
	class URepUObject* UObjectValue;

	//
	// Private Fields
	//
private:
	class UStaticMeshComponent* MeshComponent;

	// To Initialize each Server/client has diff value
	bool bLazyInitialized = false;

	int localIndex = 0;

	// Modifying bool values through editer's inspector on runtime is not applied each client.
	// even property attibute as Replicated. seems the actor not ensured 0 index (autorized)
	// it ocurred when playing same process mode
	UPROPERTY(EditAnywhere)
	bool bReplicateMoveOffset = true;

	UPROPERTY(EditAnywhere)
	bool bReplicateValue = true;

	UPROPERTY(EditAnywhere)
	bool bReplicateArray = false;

	UPROPERTY(EditAnywhere)
	bool bReplicateStructValue = false;

	UPROPERTY(EditAnywhere)
	bool bReplicateStructArray = false;

	UPROPERTY(EditAnywhere)
	bool bReplicateUObjectValue = false;

	UPROPERTY(EditAnywhere)
	bool bReplicateUObjectReference = false;

	UPROPERTY(EditAnywhere)
	bool bNotifyClient = false;

	UPROPERTY(EditAnywhere)
	bool bLogNetInfo = false;
};