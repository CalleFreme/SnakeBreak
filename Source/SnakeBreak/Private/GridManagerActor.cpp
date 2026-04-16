// Fill out your copyright notice in the Description page of Project Settings.


#include "GridManagerActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"

// Sets default values
AGridManagerActor::AGridManagerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	FloorInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>("FloorInstances");
	FloorInstances->SetupAttachment(RootComponent);
	FloorInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WallInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>("WallInstances");
	WallInstances->SetupAttachment(RootComponent);
	WallInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AGridManagerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	RebuildGrid();
}

void AGridManagerActor::BeginPlay()
{
	Super::BeginPlay();

	// Optional: safety rebuild in case something was not refreshed in-editor
	RebuildGrid();
}

void AGridManagerActor::RebuildGrid()
{
	RebuildBlockedCells();
	RebuildVisualInstances();
}

void AGridManagerActor::RebuildBlockedCells()
{
	BlockedCells.Empty();

	if (!bGenerateBorderWalls)
	{
		return;
	}

	for (int32 X = 0; X < GridDimensions.X; ++X)
	{
		BlockedCells.Add(FIntPoint(X, 0));
		BlockedCells.Add(FIntPoint(X, GridDimensions.Y - 1));
	}

	for (int32 Y = 0; Y < GridDimensions.Y; ++Y)
	{
		BlockedCells.Add(FIntPoint(0, Y));
		BlockedCells.Add(FIntPoint(GridDimensions.X - 1, Y));
	}
}

void AGridManagerActor::ClearVisualInstances()
{
	if (FloorInstances)
	{
		FloorInstances->ClearInstances();
	}

	if (WallInstances)
	{
		WallInstances->ClearInstances();
	}
}

void AGridManagerActor::RebuildVisualInstances()
{
	ClearVisualInstances();

	// If no mesh is assigned to the ISM components, nothing will render
	if (!FloorInstances || !WallInstances)
	{
		return;
	}

	for (int32 X = 0; X < GridDimensions.X; ++X)
	{
		for (int32 Y = 0; Y < GridDimensions.Y; ++Y)
		{
			const FIntPoint Cell(X, Y);
			const bool bBlocked = BlockedCells.Contains(Cell);

			const FVector BaseWorld = GridToWorld(Cell);

			if (bGenerateFloor)
			{
				const FVector FloorLocation = BaseWorld + FVector(0.f, 0.f, FloorZOffset);
				FTransform FloorTransform(FRotator::ZeroRotator, FloorLocation, InstanceScale);
				FloorInstances->AddInstance(FloorTransform);
			}

			if (bBlocked)
			{
				const FVector WallLocation = BaseWorld + FVector(0.f, 0.f, WallZOffset);
				FTransform WallTransform(FRotator::ZeroRotator, WallLocation, InstanceScale);
				WallInstances->AddInstance(WallTransform);
			}
		}

	}
}

FVector AGridManagerActor::GridToWorld(const FIntPoint& GridCell) const
{
	return GridOrigin + FVector(GridCell.X * CellSize, GridCell.Y * CellSize, 0.f);
}

bool AGridManagerActor::IsInsidePlayableArea(const FIntPoint& GridCell) const
{
	return GridCell.X > 0 && GridCell.X < GridDimensions.X - 1
		&& GridCell.Y > 0 && GridCell.Y < GridDimensions.Y - 1;

	/*return static_cast<uint32>(GridCell.X) < static_cast<uint32>(GridDimensions.X)
		&& static_cast<uint32>(GridCell.Y) < static_cast<uint32>(GridDimensions.Y);*/

}

bool AGridManagerActor::IsBlockedCell(const FIntPoint& GridCell) const
{
	return BlockedCells.Contains(GridCell);
}

bool AGridManagerActor::TryGetRandomFreeCell(FIntPoint& OutCell, const TArray<FIntPoint>& ForbiddenCells, int32 MaxAttempts) const
{
	TSet<FIntPoint> ForbiddenSet(ForbiddenCells);

	for (int32 i = 0; i < MaxAttempts; ++i)
	{
		const FIntPoint Candidate(
			FMath::RandRange(1, GridDimensions.X - 2),
			FMath::RandRange(1, GridDimensions.Y - 2));

		if (BlockedCells.Contains(Candidate) || ForbiddenSet.Contains(Candidate))
		{
			continue;
		}

		OutCell = Candidate;
		return true;
	}

	return false;
}



// Called every frame
//void AGridManagerActor::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

