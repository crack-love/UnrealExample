#include "RepActor.h"
#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h" // DOREPLIFETIME
#include "Engine/ActorChannel.h" // UActorChannel
#include "Kismet/GameplayStatics.h"

#include "RepUObject.h"
#include "Log.h"

ARepActor::ARepActor()
{
	PrimaryActorTick.bCanEverTick = true;
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

	// Create SubObject
	// Only create server side object will be replicated.
	if (HasAuthority())
	{
		// Objects only replicate from server to client. If we didn't guard this
		// the client would create the object just fine but it would get replaced 
		// by the server version (more accurately the property would be replaced to 
		// point to the version from the server. The one the client allocated would 
		// eventually be garbage collected.
		// NOTE: Very important, objects Outer must be our Actor! 
		UObjectValue = CreateDefaultSubobject<URepUObject>(TEXT("SubObject"));
	}
}

void ARepActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARepActor, Value);
	DOREPLIFETIME(ARepActor, StructValue);
	DOREPLIFETIME(ARepActor, UObjectValue);

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

	if (HasAuthority())
	{
		Server_MoveNext(1);
	}
}

void ARepActor::Server_MoveNext_Implementation(int param)
{
	check(HasAuthority());

	AddActorLocalOffset(FVector::OneVector * 0.1f * param);

	Value += param;
	
	///StructValue.Value = StructValue.Value + Value;
	
	// chagne uobject's data all route act same (callback it's replicatedUsing)
	//UObjectValue->Value = UObjectValue->Value + param;
	//UObjectValue->SetValue(UObjectValue->Value + param);
	//UObjectValue->Server_SetValue(UObjectValue->Value + param);
	//UObjectValue->Server_SetValue_Implementation(UObjectValue->Value + param);
	
	// change uobject's reference is call OnRep_UObject all route acting same
	//UObjectValue = static_cast<URepUObject*>(UGameplayStatics::SpawnObject(URepUObject::StaticClass(), this));
	//UObjectValue = NewObject<URepUObject>(this);

	Client_Notify();
}

bool ARepActor::Server_MoveNext_Validate(int param)
{
	return true;
}

void ARepActor::Client_Notify_Implementation()
{
	check(GIsClient);

	GEngine->AddOnScreenDebugMessage(1, 0.5f, FColor::Green, FString::Printf(TEXT("CNotify: %f"), Value));
}

void ARepActor::OnRep_Value()
{
	check(GIsClient);

	GEngine->AddOnScreenDebugMessage(2, 0.5f, FColor::Green, FString::Printf(TEXT("OnRep: %f"), Value));
}

void ARepActor::OnRep_Struct()
{
	check(GIsClient);

	FLog::Log(TEXTF("ARepActor.OnRep_Struct %f", StructValue.Value));
}

void ARepActor::OnRep_UObject()
{
	check(GIsClient);

	if (UObjectValue != nullptr)
	{
		FLog::Log(TEXTF("ARepActor.OnRep_UObject %f", UObjectValue->Value));
	}
}