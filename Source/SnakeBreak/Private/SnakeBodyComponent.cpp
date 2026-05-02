#include "SnakeBodyComponent.h"

#include "GridManagerActor.h"
#include "Hazard.h"
#include "SnakePawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

USnakeBodyComponent::USnakeBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USnakeBodyComponent::SetGridContext(
	AGridManagerActor* NewGridManager,
	float NewCellSize,
	const FVector& NewGridOrigin)
{
	GridManager = NewGridManager;
	CellSize = NewCellSize;
	GridOrigin = NewGridOrigin;
}

void USnakeBodyComponent::ResetBody(
	int32 NumSegments,
	const FIntPoint& HeadCell,
	const FIntPoint& BackwardOffset,
	float Alpha)
{
	ClearBody();
	AddInitialBodySegments(NumSegments, HeadCell, BackwardOffset);
	UpdateSegments(Alpha);
}

void USnakeBodyComponent::ClearBody()
{
	CurrentBodyGridPositions.Empty();
	PreviousBodyGridPositions.Empty();
	TargetBodyGridPositions.Empty();
	PendingGrowth = 0;
	ClearBodyVisuals();
	ClearBodySegmentColliders();
}

void USnakeBodyComponent::PrepareMoveTargets(const FIntPoint& CurrentHeadCell)
{
	// At step start, snapshot the old body and describe where each segment
	// should be when the head reaches the next cell.
	PreviousBodyGridPositions = CurrentBodyGridPositions;
	TargetBodyGridPositions.Empty(CurrentBodyGridPositions.Num());

	if (CurrentBodyGridPositions.Num() == 0)
	{
		return;
	}

	TargetBodyGridPositions.Add(CurrentHeadCell);

	for (int32 Index = 1; Index < CurrentBodyGridPositions.Num(); ++Index)
	{
		TargetBodyGridPositions.Add(CurrentBodyGridPositions[Index - 1]);
	}
}

void USnakeBodyComponent::FinishMoveStep(const FIntPoint& OldHeadCell)
{
	// After the head enters a new cell, the old head cell becomes segment 0.
	// Growth consumes one pending point by keeping the tail for this step.
	if (CurrentBodyGridPositions.Num() > 0 || PendingGrowth > 0)
	{
		CurrentBodyGridPositions.Insert(OldHeadCell, 0);

		if (PendingGrowth > 0)
		{
			--PendingGrowth;
		}
		else
		{
			CurrentBodyGridPositions.RemoveAt(CurrentBodyGridPositions.Num() - 1);
		}
	}

	UpdateSegments(1.f);
}

void USnakeBodyComponent::UpdateSegments(float Alpha)
{
	UpdateBodyVisuals(Alpha);
	UpdateBodySegmentColliders(Alpha);
}

void USnakeBodyComponent::Grow(int32 Amount)
{
	PendingGrowth += FMath::Max(Amount, 0);
}

void USnakeBodyComponent::TrimTail(int32 HitSegmentIndex)
{
	if (!CurrentBodyGridPositions.IsValidIndex(HitSegmentIndex))
	{
		return;
	}

	const int32 NumToRemove = CurrentBodyGridPositions.Num() - HitSegmentIndex;
	CurrentBodyGridPositions.RemoveAt(HitSegmentIndex, NumToRemove);

	// Rebase interpolation data onto the new body so visuals/colliders snap
	// cleanly after a hazard cuts the tail.
	PreviousBodyGridPositions = CurrentBodyGridPositions;
	TargetBodyGridPositions = CurrentBodyGridPositions;
	PendingGrowth = 0;

	UpdateSegments(1.f);
}

bool USnakeBodyComponent::WouldHitSelf(const FIntPoint& NextCell) const
{
	const bool bTailWillStayThisStep = PendingGrowth > 0;

	for (int32 Index = 0; Index < CurrentBodyGridPositions.Num(); ++Index)
	{
		const bool bIsTail = Index == CurrentBodyGridPositions.Num() - 1;
		if (!bTailWillStayThisStep && bIsTail)
		{
			continue;
		}

		if (CurrentBodyGridPositions[Index] == NextCell)
		{
			return true;
		}
	}

	return false;
}

void USnakeBodyComponent::GetProjectedOccupiedCellsAfterMove(
	const FIntPoint& CurrentHeadCell,
	const FIntPoint& NextCell,
	TSet<FIntPoint>& OutOccupiedCells) const
{
	OutOccupiedCells.Reset();
	OutOccupiedCells.Add(NextCell);

	if (CurrentBodyGridPositions.Num() == 0 && PendingGrowth <= 0)
	{
		return;
	}

	OutOccupiedCells.Add(CurrentHeadCell);

	const bool bTailWillStayThisStep = PendingGrowth > 0;
	const int32 BodyCellsToKeep = bTailWillStayThisStep
		? CurrentBodyGridPositions.Num()
		: CurrentBodyGridPositions.Num() - 1;

	for (int32 Index = 0; Index < BodyCellsToKeep; ++Index)
	{
		OutOccupiedCells.Add(CurrentBodyGridPositions[Index]);
	}
}

void USnakeBodyComponent::DrawDebugBody() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const USphereComponent* Collider : BodySegmentColliders)
	{
		if (!IsValid(Collider))
		{
			continue;
		}

		DrawDebugSphere(
			World,
			Collider->GetComponentLocation(),
			Collider->GetScaledSphereRadius(),
			12,
			FColor::Yellow,
			false,
			-1.f,
			0,
			1.5f);
	}
}

FVector USnakeBodyComponent::GridToWorldLocation(const FIntPoint& GridPosition) const
{
	if (GridManager)
	{
		return GridManager->GridToWorld(GridPosition);
	}

	return GridOrigin + FVector(GridPosition.X * CellSize, GridPosition.Y * CellSize, 0.f);
}

void USnakeBodyComponent::AddInitialBodySegments(
	int32 NumSegments,
	const FIntPoint& HeadCell,
	const FIntPoint& BackwardOffset)
{
	CurrentBodyGridPositions.Empty();

	FIntPoint NextBodyCell = HeadCell + BackwardOffset;

	for (int32 Index = 0; Index < NumSegments; ++Index)
	{
		CurrentBodyGridPositions.Add(NextBodyCell);
		NextBodyCell += BackwardOffset;
	}
}

void USnakeBodyComponent::HandleBodySegmentOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	AActor* Owner = GetOwner();
	if (!Owner || !OtherActor || OtherActor == Owner)
	{
		return;
	}

	AHazard* Hazard = Cast<AHazard>(OtherActor);
	if (!Hazard)
	{
		return;
	}

	const int32 HitSegmentIndex = BodySegmentColliders.IndexOfByKey(
		Cast<USphereComponent>(OverlappedComponent));
	if (HitSegmentIndex == INDEX_NONE)
	{
		return;
	}

	Hazard->EliminateTarget(Owner, HitSegmentIndex);
}

void USnakeBodyComponent::UpdateBodyVisuals(float Alpha)
{
	EnsureBodySegmentMeshCount();

	for (int32 Index = 0; Index < BodySegmentMeshes.Num(); ++Index)
	{
		UStaticMeshComponent* SegmentMesh = BodySegmentMeshes[Index];
		if (!SegmentMesh || !CurrentBodyGridPositions.IsValidIndex(Index))
		{
			continue;
		}

		const FIntPoint StartCell = PreviousBodyGridPositions.IsValidIndex(Index)
			? PreviousBodyGridPositions[Index]
			: CurrentBodyGridPositions[Index];

		const FIntPoint TargetCell = TargetBodyGridPositions.IsValidIndex(Index)
			? TargetBodyGridPositions[Index]
			: CurrentBodyGridPositions[Index];

		FVector NewWorldLocation = FMath::Lerp(
			GridToWorldLocation(StartCell),
			GridToWorldLocation(TargetCell),
			Alpha);

		NewWorldLocation.Z += BodySegmentMeshZOffset;
		SegmentMesh->SetWorldLocation(NewWorldLocation);
	}
}

void USnakeBodyComponent::UpdateBodySegmentColliders(float Alpha)
{
	EnsureBodySegmentColliderCount();

	for (int32 Index = 0; Index < BodySegmentColliders.Num(); ++Index)
	{
		USphereComponent* Collider = BodySegmentColliders[Index];
		if (!IsValid(Collider) || !CurrentBodyGridPositions.IsValidIndex(Index))
		{
			continue;
		}

		const FIntPoint StartCell = PreviousBodyGridPositions.IsValidIndex(Index)
			? PreviousBodyGridPositions[Index]
			: CurrentBodyGridPositions[Index];

		const FIntPoint TargetCell = TargetBodyGridPositions.IsValidIndex(Index)
			? TargetBodyGridPositions[Index]
			: CurrentBodyGridPositions[Index];

		const FVector NewWorldLocation = FMath::Lerp(
			GridToWorldLocation(StartCell),
			GridToWorldLocation(TargetCell),
			Alpha);

		Collider->SetWorldLocation(NewWorldLocation);
	}
}

void USnakeBodyComponent::EnsureBodySegmentMeshCount()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	while (BodySegmentMeshes.Num() < CurrentBodyGridPositions.Num())
	{
		const int32 SegmentIndex = BodySegmentMeshes.Num();
		const FName ComponentName = *FString::Printf(TEXT("BodySegmentMesh_%d"), SegmentIndex);

		UStaticMeshComponent* NewSegmentMesh = NewObject<UStaticMeshComponent>(Owner, ComponentName);
		if (!NewSegmentMesh)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create body segment mesh component."));
			return;
		}

		NewSegmentMesh->SetupAttachment(Owner->GetRootComponent());
		NewSegmentMesh->RegisterComponent();
		NewSegmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewSegmentMesh->SetSimulatePhysics(false);
		NewSegmentMesh->SetRelativeScale3D(BodySegmentMeshScale);

		if (BodySegmentMeshAsset)
		{
			NewSegmentMesh->SetStaticMesh(BodySegmentMeshAsset);
		}

		if (BodySegmentMaterial)
		{
			NewSegmentMesh->SetMaterial(0, BodySegmentMaterial);
		}

		BodySegmentMeshes.Add(NewSegmentMesh);
	}

	while (BodySegmentMeshes.Num() > CurrentBodyGridPositions.Num())
	{
		if (UStaticMeshComponent* LastSegment = BodySegmentMeshes.Last())
		{
			LastSegment->DestroyComponent();
		}

		BodySegmentMeshes.Pop();
	}
}

void USnakeBodyComponent::EnsureBodySegmentColliderCount()
{
	while (BodySegmentColliders.Num() < CurrentBodyGridPositions.Num())
	{
		const int32 SegmentIndex = BodySegmentColliders.Num();
		USphereComponent* NewCollider = CreateBodySegmentCollider(
			SegmentIndex,
			CurrentBodyGridPositions[SegmentIndex]);

		BodySegmentColliders.Add(NewCollider);
	}

	while (BodySegmentColliders.Num() > CurrentBodyGridPositions.Num())
	{
		if (USphereComponent* LastCollider = BodySegmentColliders.Last())
		{
			LastCollider->DestroyComponent();
		}

		BodySegmentColliders.Pop();
	}
}

USphereComponent* USnakeBodyComponent::CreateBodySegmentCollider(
	int32 SegmentIndex,
	const FIntPoint& GridCell)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	USphereComponent* NewCollider = NewObject<USphereComponent>(
		Owner,
		USphereComponent::StaticClass(),
		*FString::Printf(TEXT("BodySegmentCollider_%d"), SegmentIndex));

	if (!NewCollider)
	{
		return nullptr;
	}

	NewCollider->SetupAttachment(Owner->GetRootComponent());
	NewCollider->RegisterComponent();
	NewCollider->SetSphereRadius(BodySegmentColliderRadius);
	NewCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	NewCollider->SetGenerateOverlapEvents(true);
	NewCollider->SetCollisionObjectType(ECC_Pawn);
	NewCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	NewCollider->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	NewCollider->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	NewCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	NewCollider->OnComponentBeginOverlap.AddDynamic(
		this,
		&USnakeBodyComponent::HandleBodySegmentOverlap);
	NewCollider->SetWorldLocation(GridToWorldLocation(GridCell));

	return NewCollider;
}

void USnakeBodyComponent::ClearBodyVisuals()
{
	for (UStaticMeshComponent* SegmentMesh : BodySegmentMeshes)
	{
		if (SegmentMesh)
		{
			SegmentMesh->DestroyComponent();
		}
	}

	BodySegmentMeshes.Empty();
}

void USnakeBodyComponent::ClearBodySegmentColliders()
{
	for (USphereComponent* Collider : BodySegmentColliders)
	{
		if (Collider)
		{
			Collider->DestroyComponent();
		}
	}

	BodySegmentColliders.Empty();
}
