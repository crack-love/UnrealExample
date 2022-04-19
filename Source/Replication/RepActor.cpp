#include "RepActor.h"
#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h" // DOREPLIFETIME
#include "Engine/ActorChannel.h" // UActorChannel
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameState.h"
#include "DrawDebugHelpers.h"

#include "RepUObject.h"
#include "Log.h"

ARepActor::ARepActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.6f;
	SetReplicates(true);
	SetReplicateMovement(true);

	// Other Options
	//AActor::NetUpdateFrequency - Used to determine how often an actor replicates
	//AActor::PreReplication - Called before any replicaion occurs
	// - For any actor that passes these initial checks, AActor::PreReplication is called.
	// - PreReplication is a place where you can decide if you want properties to replicate for connections.
	// - Use the DOREPLIFETIME_ACTIVE_OVERRIDE for this.
	//AActor::bOnlyRelevantToOwner - True if this actor only replicates to owner
	//AActor::IsRelevancyOwnerFor - Called to determine relevancy when bOnlyRelevantToOwner is true
	//AActor::IsNetRelevantFor - Called to determine relevancy when bOnlyRelevantToOwner is false

	// Create MeshComponent
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	static auto MeshFinder = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	if (MeshFinder.Succeeded())
	{
		MeshComponent->SetStaticMesh(MeshFinder.Object);
	}
	SetRootComponent(MeshComponent);

	// Create SubObjects
	// Objects only replicate from server to client. If we didn't guard this
	// the client would create the object just fine but it would get replaced 
	// by the server version (more accurately the property would be replaced to 
	// point to the version from the server. The one the client allocated would 
	// eventually be garbage collected.
	// NOTE: Very important, objects Outer must be our Actor! 
	UObjectValue = CreateDefaultSubobject<URepUObject>(TEXT("SubObject"));
	ArrayValue.Init(0, 10);
	StructValue.ArrayValue.Init(0, 10);
}

void ARepActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Initialize
	NetCullDistanceSquared = NetCullDistance * NetCullDistance;
	MeshComponent->SetCullDistance(NetCullDistance);

	// meaningless value only for screen info logging
	static int Cnt = 0;
	ScreenLogIndex = Cnt;

	Value = Cnt;
	StructValue.Value = Cnt;
	UObjectValue->Value = Cnt;
	for (int i = 0; i < ArrayValue.Num(); ++i)
	{
		ArrayValue[i] = Cnt;
		StructValue.ArrayValue[i] = Cnt;
	}
	++Cnt;
}

void ARepActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARepActor, Value);
	DOREPLIFETIME(ARepActor, StructValue);
	DOREPLIFETIME(ARepActor, UObjectValue);
	DOREPLIFETIME(ARepActor, ArrayValue);

	// Add any Blueprint properties
	// This is not required if you do not want the class to be "Blueprintable"
	if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

bool ARepActor::ReplicateSubobjects(class UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	WroteSomething |= Channel->ReplicateSubobject(UObjectValue, *Bunch, *RepFlags);

	return WroteSomething;
}

void ARepActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Only Server Side
	if (HasAuthority())
	{
		Server_Tick(1);

		if (bReplicateArray)
		{
			LogArray(TEXT("Server Array: "), ArrayValue);
		}

		if (bReplicateStructArray)
		{
			LogArray(TEXT("Server Struct Array: "), StructValue.ArrayValue);
		}
	}

	if (bLogNetInfo)
	{
		LogNetInfo();
	}

	// draw net cull range sphere
	DrawDebugSphere(GetWorld(), GetActorLocation(), NetCullDistance, 50, FColor::Green, false, PrimaryActorTick.TickInterval);
}

void ARepActor::Server_Tick_Implementation(int param)
{
	check(HasAuthority());

	if (bReplicateMoveOffset)
	{
		AddActorLocalOffset(FVector::OneVector * param);
	}

	if (bReplicateValue)
	{
		Value += param;
	}

	if (bReplicateStructValue)
	{
		StructValue.Value = StructValue.Value + param;
	}

	if (bReplicateArray)
	{
		ArrayValue[FMath::RandRange(0, 9)] = FMath::RandRange(0, 9);
	}

	if (bReplicateStructArray)
	{
		StructValue.ArrayValue[FMath::RandRange(0, 9)] = FMath::RandRange(0, 9);
	}

	// all methods are act same
	if (bReplicateUObjectValue)
	{
		UObjectValue->Value = UObjectValue->Value + param;
		//UObjectValue->SetValue(UObjectValue->Value + param);
		//UObjectValue->Server_SetValue(UObjectValue->Value + param);
		//UObjectValue->Server_SetValue_Implementation(UObjectValue->Value + param);
	}

	// change uobject's reference callback OnRep_UObject
	// all methods are act same
	if (bReplicateUObjectReference)
	{
		UObjectValue = NewObject<URepUObject>(this);
		//UObjectValue = static_cast<URepUObject*>(UGameplayStatics::SpawnObject(URepUObject::StaticClass(), this));
	}

	if (bNotifyClient)
	{
		Client_Notify();
	}
}

bool ARepActor::Server_Tick_Validate(int param)
{
	check(HasAuthority());

	return true;
}

void ARepActor::Client_Notify_Implementation()
{
	check((GetLocalRole() & ROLE_SimulatedProxy) != ROLE_None);

	LogMessage(TEXT("ARepActor.Client_Notify"));
}

void ARepActor::OnRep_Value()
{
	check(GetLocalRole() != ROLE_Authority);
	
	LogMessage(TEXTFP("ARepActor.OnRep_Value Value: %f", Value));
}

void ARepActor::OnRep_Struct()
{
	check(!HasAuthority());

	if (bReplicateStructValue)
	{
		LogMessage(TEXTFP("ARepActor.OnRep_Struct Value: %f", StructValue.Value));
	}

	if (bReplicateStructArray)
	{
		LogArray(TEXT("ARepActor.OnRep_Struct Array: "), StructValue.ArrayValue);
	}
}

void ARepActor::OnRep_UObject()
{
	check(!HasAuthority());

	LogMessage(TEXTFP("ARepActor.OnRep_UObject UObject.Value: %f", UObjectValue == nullptr ? -1.f : Value));
}

void ARepActor::OnRep_Array()
{
	check(!HasAuthority());

	LogArray(TEXT("ARepActor.OnRep_Array Array: "), ArrayValue);
}

void ARepActor::LogMessage(const TCHAR* PMsg)
{
	FString Msg;
	Msg.Appendf(TEXT("%s: "), *GetWorld()->GetGameInstance()->GetName());
	Msg.Appendf(TEXT("%s"), PMsg);
	FLog::Log(Msg);
}

void ARepActor::LogArray(const TCHAR* PMsg, TArray<int32>& PArray)
{
	FString Msg(PMsg);

	for (int32 i = 0; i < PArray.Num(); ++i)
	{
		Msg.Appendf(TEXT("%d "), PArray[i]);
	}

	FLog::Log(Msg);
}

void ARepActor::LogNetInfo()
{
	auto player = GEngine->GetFirstGamePlayer(GetWorld());
	FString GameInsStr = player->GetGameInstance()->GetName();
	FString LocalRoleStr = UEnum::GetValueAsString(GetLocalRole());
	FString NetDriverNameStr = GetWorld()->GetNetDriver()->GetName();

	FString Msg;
	Msg.Appendf(TEXT("GameIns:%s "), *GameInsStr);
	Msg.Appendf(TEXT("NetDriver:%s "), *NetDriverNameStr);
	Msg.Appendf(TEXT("HasAuth:%d "), HasAuthority());
	Msg.Appendf(TEXT("bRepValue:%d "), bReplicateValue);
	Msg.Appendf(TEXT("LocalRole:%s "), *LocalRoleStr);
	Msg.Appendf(TEXT("NetCull:%f "), NetCullDistanceSquared);
	FLog::Screen(Msg, ScreenLogIndex, 3.f);
}