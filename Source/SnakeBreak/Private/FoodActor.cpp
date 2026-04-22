// Fill out your copyright notice in the Description page of Project Settings.


#include "FoodActor.h"
#include "SnakePawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AFoodActor::AFoodActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(CollisionRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	FoodMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FoodMesh"));
	FoodMesh->SetupAttachment(RootComponent);
	FoodMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FoodMesh->SetSimulatePhysics(false);
}

// Called when the game starts or when spawned
void AFoodActor::BeginPlay()
{
	Super::BeginPlay();
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AFoodActor::HandleFoodOverlap);
}

// Called every frame
void AFoodActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFoodActor::HandleFoodOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherCamp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bIsActive) return;
	
	if (ASnakePawn* SnakePawn = Cast<ASnakePawn>(OtherActor))
	{
		bIsActive = false;
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SnakePawn->HandleFoodOverlap(this);
	}
}

void AFoodActor::DeactivateFood()
{
	bIsActive = false;
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFoodActor::RespawnFood(const FIntPoint& NewGridPosition, const FVector& NewWorldLocation)
{
	FoodGridPosition = NewGridPosition;
	SetActorLocation(NewWorldLocation, false, nullptr, ETeleportType::TeleportPhysics);

	bIsActive = true;
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetGenerateOverlapEvents(true);
	CollisionSphere->UpdateOverlaps();
}
