#include "RepActor.h"
#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h" // DOREPLIFETIME
#include "Engine/ActorChannel.h" // UActorChannel
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameState.h"

#include "RepUObject.h"
#include "Log.h"

ARepActor::ARepActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1;
	SetReplicates(true);
	SetReplicateMovement(true);

	// Create MeshComponent
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	static auto MeshFinder = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	if (MeshFinder.Succeeded())
	{
		MeshComponent->SetStaticMesh(MeshFinder.Object);
	}
	SetRootComponent(MeshComponent);

	// Create SubObjects
	// Only create server side object that will be replicated.
	if (HasAuthority())
	{
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

bool ARepActor::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	WroteSomething |= Channel->ReplicateSubobject(UObjectValue, *Bunch, *RepFlags);

	return WroteSomething;
}

void ARepActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Set Initial Value Diffect each Client
	static int Cnt = 0;
	if (!bLazyInitialized)
	{
		bLazyInitialized = true;

		localIndex = Cnt;
		++Cnt;
		Value = Cnt;
		StructValue.Value = Cnt;
		UObjectValue->Value = Cnt;
		for (int i = 0; i < ArrayValue.Num(); ++i)
		{
			ArrayValue[i] = Cnt;
			StructValue.ArrayValue[i] = Cnt;
		}
	}

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

	// Log Net Info
	if (bLogNetInfo)
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
		FLog::Screen(Msg, localIndex, 3.f);
	}
}

void ARepActor::Server_Tick_Implementation(int param)
{
	check(HasAuthority());

	if (bReplicateMoveOffset)
	{
		AddActorLocalOffset(FVector::OneVector * 0.1f * param);
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

	FLog::Log(TEXTF("ARepActor.Client_Notify %s", *GetWorld()->GetFirstPlayerController()->GetName()));
}

void ARepActor::OnRep_Value()
{
	check(GetLocalRole() != ROLE_Authority);
	
	FLog::Log(TEXTF("ARepActor.OnRep_Value Value: %f", Value));
}

void ARepActor::OnRep_Struct()
{
	check(!HasAuthority());

	if (bReplicateStructValue)
	{
		FLog::Log(TEXTF("ARepActor.OnRep_Struct Value: %f", StructValue.Value));
	}

	if (bReplicateStructArray)
	{
		LogArray(TEXT("ARepActor.OnRep_Struct Array: "), StructValue.ArrayValue);
	}
}

void ARepActor::OnRep_UObject()
{
	check(!HasAuthority());

	FLog::Log(TEXTF("ARepActor.OnRep_UObject UObject.Value: %f", UObjectValue == nullptr ? -1.f : Value));
}

void ARepActor::OnRep_Array()
{
	check(!HasAuthority());

	LogArray(TEXT("ARepActor.OnRep_Array Array: "), ArrayValue);
}

void ARepActor::LogArray(TCHAR* PMsg, TArray<int32>& PArray)
{
	FString Msg(PMsg);

	for (int32 i = 0; i < PArray.Num(); ++i)
	{
		Msg.Appendf(TEXT("%d "), PArray[i]);
	}

	FLog::Log(Msg);
}