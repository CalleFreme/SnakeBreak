#include "Hazard.h"
#include "HazardTarget.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

AHazard::AHazard()
{
	PrimaryActorTick.bCanEverTick = false;

	HazardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HazardMesh"));
	HazardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HazardMesh->SetGenerateOverlapEvents(false);
}

void AHazard::EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex)
{
	EliminateTargetActor(TargetActor);
}

bool AHazard::HasAssignedVisualMesh() const
{
	return HazardMesh && HazardMesh->GetStaticMesh();
}

void AHazard::SetHazardActive(bool bNewActive)
{
	bHazardActive = bNewActive;
	SetActorHiddenInGame(!bHazardActive);
	SetActorEnableCollision(bHazardActive);
}

void AHazard::BeginPlay()
{
	Super::BeginPlay();
}

void AHazard::ConfigureHazardCollision(UPrimitiveComponent* CollisionComponent) const
{
	if (!CollisionComponent)
	{
		return;
	}

	// Hazards are overlap-only gameplay volumes. Grid movement owns placement;
	// physics blocking would make collision behavior less predictable.
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AHazard::EliminateTargetActor(AActor* TargetActor)
{
	if (TargetActor && TargetActor->GetClass()->ImplementsInterface(UHazardTarget::StaticClass()))
	{
		IHazardTarget::Execute_EliminateByHazard(TargetActor, this);
	}
}

void AHazard::TrimTargetTail(AActor* TargetActor, int32 HitSegmentIndex)
{
	if (TargetActor && TargetActor->GetClass()->ImplementsInterface(UHazardTarget::StaticClass()))
	{
		IHazardTarget::Execute_TrimTailByHazard(TargetActor, this, HitSegmentIndex);
	}
}




//
// ALaserWall
//
ALaserWall::ALaserWall()
{
	PrimaryActorTick.bCanEverTick = false;
	HazardType = EHazardType::LaserWall;
	LaserCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("LaserCollision"));
	ConfigureHazardCollision(LaserCollision);
	RootComponent = LaserCollision;
	HazardMesh->SetupAttachment(RootComponent);
	
	ToggleRateSeconds = 4.f;
}

void ALaserWall::BeginPlay()
{
	Super::BeginPlay();
	SetHazardActive(bStartsActive);
	ConfigureLaserTimer();
}

void ALaserWall::EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex)
{
	if (!IsHazardActive() || !TargetActor)
	{
		return;
	}

	if (HitSegmentIndex == HeadHitIndex)
	{
		EliminateTargetActor(TargetActor);
	}
	else
	{
		TrimTargetTail(TargetActor, HitSegmentIndex);
	}
}

void ALaserWall::InitializeLaser(float NewToggleRateSeconds)
{
	if (NewToggleRateSeconds >= 0.f)
	{
		ToggleRateSeconds = NewToggleRateSeconds;
	}

	GetWorldTimerManager().ClearTimer(ToggleTimerHandle);
	SetHazardActive(bStartsActive);
	ConfigureLaserTimer();
}

void ALaserWall::ConfigureLaserTimer()
{
	if (ToggleRateSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			ToggleTimerHandle,
			this,
			&ALaserWall::ToggleLaser,
			ToggleRateSeconds,
			true
		);
	}
}

void ALaserWall::ToggleLaser()
{
	SetHazardActive(!IsHazardActive());
}

void ALaserWall::SetHazardActive(bool bNewActive)
{
	Super::SetHazardActive(bNewActive);

	if (LaserCollision)
	{
		LaserCollision->SetCollisionEnabled(
			IsHazardActive()
			? ECollisionEnabled::QueryOnly
			: ECollisionEnabled::NoCollision);
	}
}


//
// AMovingLaserBeam
//
AMovingLaserBeam::AMovingLaserBeam()
{
	PrimaryActorTick.bCanEverTick = true;
	HazardType = EHazardType::MovingLaserBeam;

	LaserCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("LaserCollision"));
	ConfigureHazardCollision(LaserCollision);
	RootComponent = LaserCollision;
	HazardMesh->SetupAttachment(RootComponent);

	GridMovementComponent = CreateDefaultSubobject<UGridMovementComponent>(TEXT("GridMovementComponent"));
}
 
void AMovingLaserBeam::BeginPlay()
{
	Super::BeginPlay();
}


void AMovingLaserBeam::EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex)
{
	if (!TargetActor)
	{
		return;
	}

	if (HitSegmentIndex == HeadHitIndex)
	{
		EliminateTargetActor(TargetActor);
	}
	else
	{
		TrimTargetTail(TargetActor, HitSegmentIndex);
	}
}




//
// AMovingWall
//
AMovingWall::AMovingWall()
{
	PrimaryActorTick.bCanEverTick = true;
	HazardType = EHazardType::MovingWall;

	WallCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WallCollision"));
	ConfigureHazardCollision(WallCollision);
	RootComponent = WallCollision;
	HazardMesh->SetupAttachment(RootComponent);

	GridMovementComponent = CreateDefaultSubobject<UGridMovementComponent>(TEXT("GridMovementComponent"));
}
 
void AMovingWall::BeginPlay()
{
	Super::BeginPlay();
}


void AMovingWall::EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex)
{
	EliminateTargetActor(TargetActor);
}


/*void AHazard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}*/

