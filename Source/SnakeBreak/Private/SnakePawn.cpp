#include "SnakePawn.h"
#include "FoodActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "SnakeGameState.h"

ASnakePawn::ASnakePawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics; // Ensures this actor tick AFTER all physics and movement has been resolved. This is important for our snake pawn, because we want to update the snake's position and direction based on player input after all movement and collision has been processed for the frame, so that the snake's movement feels responsive and accurate to the player's input without being affected by physics or collision issues.

	// Automatically possess this pawn with the first player controller
	// AutoPossessPlayer = EAutoReceiveInput::Player0;
	// Alternatively, you would set up GameMode to spawn this pawn class and possess it, but for a simple game like this

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(CollisionSphereRadius);
	CollisionSphere->SetCollisionProfileName(TEXT("Pawn")); // Set the collision profile to "Pawn", which is a predefined profile that allows it to collide with the world and other pawns, but not block the camera or other things. This is important for our snake pawn, because we want it to be able to collide with the ground and other objects, but we don't want it to block the camera or cause weird physics interactions with the box mesh.

	SnakeHeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SnakeHeadMesh"));
	SnakeHeadMesh->SetupAttachment(RootComponent);
	SnakeHeadMesh->SetRelativeLocation(FVector::ZeroVector); // Center the mesh on the root component (collision sphere)
	// We don't want the mesh to collide with the ground or other objects, because we're going to use a separate collision sphere for that, and we want the mesh to just be a visual representation of the snake's head without affecting physics or collisions. So we set it to NoCollision, which means it won't generate any collision events or block anything. We also set SimulatePhysics to false, because we don't want the mesh to be affected by physics forces or gravity, since we're going to move it manually in the Tick function based on player input:
	SnakeHeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SnakeHeadMesh->SetSimulatePhysics(false);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 800.f;
	SpringArm->bUsePawnControlRotation = false; // fixed camera, not controller-driven
	SpringArm->SetUsingAbsoluteRotation(true);  // fixed world rotation, not relative to pawn rotation
	SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	bUseControllerRotationYaw = false;
}

void ASnakePawn::BeginPlay()
{
	Super::BeginPlay();
	
	if (!GridManager)
	{
		CacheGridManager();
		if (!GridManager)
		{
			UE_LOG(LogTemp, Error, TEXT("SnakePawn could not find GridManager in world."))
		}
		else
		{
			GridDimensions = GridManager->GridDimensions;
		}
	}
	
	RequestedDirection = CurrentDirection;
	UpdateDirection(CurrentDirection);	

	if (bUseGridMovement)
	{
		CurrentGridPosition = GetClampedStartGridPosition();
		PendingNextGridPosition = CurrentGridPosition;
		
		const FVector StartLocation = GridToWorldLocation(CurrentGridPosition);
		SetActorLocation(StartLocation);

		StepStartWorldLocation = StartLocation;
		StepTargetWorldLocation = StartLocation;
		MoveInterpolationProgress = 0.f;
		bIsMovingToTarget = false;

		CurrentBodyGridPositions.Empty();
		PreviousBodyGridPositions.Empty();
		AddInitialBodySegments(InitialBodyLength);
		EnsureBodySegmentMeshCount();
		UpdateBodyVisuals(0.f);
	}
	else
	{
		FVector StartLocation = GetActorLocation();
		StartLocation.Z += 15.f;
		SetActorLocation(StartLocation);
	}
}

void ASnakePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	if (bUseGridMovement)
	{
		// Interpolate towards the target
		TickGridMovement(DeltaTime);
	}
	else
	{
		HandleDirectionChange();
		TickFreeMovement(DeltaTime);
	}

	DrawDebugInfo();
}

void ASnakePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	SetupEnhancedInput();
}

void ASnakePawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	SetupEnhancedInput();
}

void ASnakePawn::SetupEnhancedInput()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (InputMapping)
				{
					Subsystem->AddMappingContext(InputMapping, 0); // We add the Input Mapping Context to the player's Enhanced Input Subsystem, with a priority of 0 (higher priority contexts will override lower priority ones if they have overlapping bindings)
				}
			}
		}
	}
}

// Called to bind functionality to input
void ASnakePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (TurnUpAction)
		{
			EnhancedInputComponent->BindAction(TurnUpAction, ETriggerEvent::Triggered, this, &ASnakePawn::Input_TryTurnUp);
		}
		if (TurnDownAction)
		{
			EnhancedInputComponent->BindAction(TurnDownAction, ETriggerEvent::Triggered, this, &ASnakePawn::Input_TryTurnDown);
		}

		if (TurnLeftAction)
		{
			EnhancedInputComponent->BindAction(TurnLeftAction, ETriggerEvent::Triggered, this, &ASnakePawn::Input_TryTurnLeft);
		}
		if (TurnRightAction)
		{
			EnhancedInputComponent->BindAction(TurnRightAction, ETriggerEvent::Triggered, this, &ASnakePawn::Input_TryTurnRight);
		}
	}

}

void ASnakePawn::Input_TryTurnUp(const FInputActionValue& Value)
{
	bool bPressed = Value.Get<bool>();
	if (bPressed)
	{

		RequestedDirection = ESnakeDirection::Up;
	}
}

void ASnakePawn::Input_TryTurnDown(const FInputActionValue& Value)
{
	bool bPressed = Value.Get<bool>();
	if (bPressed)
	{
		RequestedDirection = ESnakeDirection::Down;
	}
}

void ASnakePawn::Input_TryTurnLeft(const FInputActionValue& Value)
{
	bool bPressed = Value.Get<bool>();
	if (bPressed)
	{
		RequestedDirection = ESnakeDirection::Left;
	}
}

void ASnakePawn::Input_TryTurnRight(const FInputActionValue& Value)
{
	bool bPressed = Value.Get<bool>();
	if (bPressed)
	{
		RequestedDirection = ESnakeDirection::Right;
	}
}

FVector ASnakePawn::GetVectorFromDirection(ESnakeDirection Direction) const
{
	switch (Direction)
	{
	case ESnakeDirection::Up:     return FVector::ForwardVector;  // +X
	case ESnakeDirection::Down:   return -FVector::ForwardVector; // -X
	case ESnakeDirection::Left:   return -FVector::RightVector;   // -Y
	case ESnakeDirection::Right:  return FVector::RightVector;    // +Y
	default:                    return FVector::ZeroVector;     // Should never happen, but we return zero just in case
	}
}

void ASnakePawn::TickGridMovement(float DeltaTime)
{
	if (MoveStepTime <= 0.f)
	{
		return;
	}

	if (!bIsMovingToTarget)
	{
		// If at target, check for direction change and update target location
		StartNewMoveStep();
		if (!bIsMovingToTarget)
		{
			return; // If we failed to start a new move step (e.g. because we hit a wall), we exit early and don't try to interpolate or update the location
		}
	}

	MoveInterpolationProgress += DeltaTime / MoveStepTime;
	const float Alpha = FMath::Clamp(MoveInterpolationProgress, 0.f, 1.f);

	// Use Lerp for constant speed across the cell
	const FVector NewHeadLocation = FMath::Lerp(StepStartWorldLocation, StepTargetWorldLocation, Alpha);
	SetActorLocation(NewHeadLocation, false);

	UpdateBodyVisuals(Alpha); // Since we have now moved the head, we also want to update the body segment locations to follow the head smoothly, so we call UpdateBodyVisuals with the current interpolation alpha to position the body segments correctly between their previous and current grid positions.

	if (Alpha >= 1.f)
	{
		// We've reached the target grid location
		FinishMoveStep();
	}
}

void ASnakePawn::StartNewMoveStep()
{
	HandleDirectionChange();

	const FIntPoint GridOffset = DirectionToGridOffset(CurrentDirection);
	PendingNextGridPosition = CurrentGridPosition + GridOffset;

	if (WouldHitWall(PendingNextGridPosition) || WouldHitSelf(PendingNextGridPosition))
	{
		HandleSnakeDeath();
		return;
	}

	// Cache the START and the END for this specific step
	PreviousBodyGridPositions = CurrentBodyGridPositions;
	TargetBodyGridPositions.Empty(CurrentBodyGridPositions.Num());
	
	if (CurrentBodyGridPositions.Num() > 0)
	{
		// Segment 0's target is always the current Head position
		TargetBodyGridPositions.Add(CurrentGridPosition);
		
		// Every other segment follows the previous segment's current position
		for (int32 i = 1; i < CurrentBodyGridPositions.Num(); i++)
		{
			TargetBodyGridPositions.Add(CurrentBodyGridPositions[i - 1]);
		}
	}
	
	StepStartWorldLocation = GridToWorldLocation(CurrentGridPosition);
	StepTargetWorldLocation = GridToWorldLocation(PendingNextGridPosition);
	MoveInterpolationProgress = 0.f; // Since we're just starting to move towards the new target, we reset the interpolation progress to 0
	bIsMovingToTarget = true;
}

void ASnakePawn::FinishMoveStep()
{
	// We've reached the target grid location, so we update our current grid position to the pending next grid position, and we can also update the body positions now that we've officially moved into the new cell:
	const FIntPoint OldHeadGridPosition = CurrentGridPosition;
	CurrentGridPosition = PendingNextGridPosition;

	// Body follows where the head / previous segment used to be
	if (CurrentBodyGridPositions.Num() > 0 || PendingGrowth > 0)
	{
		CurrentBodyGridPositions.Insert(OldHeadGridPosition, 0);
		if (PendingGrowth > 0)
		{
			PendingGrowth--;
		}
		else
		{
			CurrentBodyGridPositions.RemoveAt(CurrentBodyGridPositions.Num() - 1);
		}
	}
	
	SetActorLocation(StepTargetWorldLocation, false);
	UpdateBodyVisuals(1.f);
	bIsMovingToTarget = false;
}

void ASnakePawn::UpdateBodyVisuals(float Alpha)
{
	EnsureBodySegmentMeshCount();

	for (int32 i = 0; i < BodySegmentMeshes.Num(); ++i)
	{
		if (!BodySegmentMeshes[i]) continue;

		// Guaranteed targets from the start of the move
		FIntPoint StartCell = PreviousBodyGridPositions.IsValidIndex(i) ?
		PreviousBodyGridPositions[i] : CurrentBodyGridPositions[i];
		
		// Target cell = where this segment should be after the current step
		FIntPoint TargetCell = TargetBodyGridPositions.IsValidIndex(i) ?
		TargetBodyGridPositions[i] : StartCell;

		const FVector StartWorldLocation = GridToWorldLocation(StartCell);
		const FVector TargetWorldLocation = GridToWorldLocation(TargetCell);
		
		// This Lerp is now stable
		const FVector NewWorldLocation = FMath::Lerp(StartWorldLocation, TargetWorldLocation, Alpha);

		BodySegmentMeshes[i]->SetWorldLocation(NewWorldLocation);
	}
}

void ASnakePawn::EnsureBodySegmentMeshCount()
{
	// Add missing segments
	while (BodySegmentMeshes.Num() < CurrentBodyGridPositions.Num())
	{
		const int32 SegmentIndex = BodySegmentMeshes.Num();

		FName ComponentName = *FString::Printf(TEXT("BodySegmentMesh_%d"), SegmentIndex); // This is a unique name for the new mesh component, based on the current number of body segment meshes. This is important for Unreal Engine's component system, because each component needs to have a unique name within the actor, and by using the index we ensure that each new body segment mesh gets a unique name like "BodySegmentMesh_0", "BodySegmentMesh_1", etc.
		// Why is ComponentName a pointer? Because the constructor for UStaticMeshComponent takes a FName for the component name, and FName can be implicitly converted to a const FName& (which is what the constructor expects), so we can just pass ComponentName directly to the constructor. We also use NewObject to create the component, which is the recommended way to create components at runtime in Unreal Engine, because it properly initializes the component and registers it with the actor.
		UStaticMeshComponent* NewSegmentMesh = NewObject<UStaticMeshComponent>(this, ComponentName); // "this" refers to the ASnakePawn instance. We create a new UStaticMeshComponent as a child of the snake pawn, and we give it a unique name based on the current number of body segment meshes.

		if (!NewSegmentMesh)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create body segment mesh component"));
			return;
		}

		NewSegmentMesh->SetupAttachment(RootComponent);
		NewSegmentMesh->RegisterComponent(); // We need to register the component so that it becomes part of the actor and is rendered in the world. This is important for our snake pawn, because we want the new body segment mesh to appear in the game when it's created, and registering it ensures that it will be visible and properly attached to the snake pawn.
		NewSegmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewSegmentMesh->SetSimulatePhysics(false);
		NewSegmentMesh->SetRelativeScale3D(BodySegmentMeshScale);

		if (BodySegmentMeshAsset)
		{
			NewSegmentMesh->SetStaticMesh(BodySegmentMeshAsset);
		}

		if (BodySegmentMaterial)
		{
			NewSegmentMesh->SetMaterial(0, BodySegmentMaterial); // 0 is the material index, which means we're setting the first material slot of the mesh.
		}

		BodySegmentMeshes.Add(NewSegmentMesh);
	}

	// Remove extra segment meshes.
	while (BodySegmentMeshes.Num() > CurrentBodyGridPositions.Num())
	{
		if (UStaticMeshComponent* LastSegment = BodySegmentMeshes.Last())
		{
			LastSegment->DestroyComponent();
		}
		BodySegmentMeshes.Pop();
	}
}

void ASnakePawn::ClearBodyVisuals()
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

void ASnakePawn::AddInitialBodySegments(int32 NumSegments)
{
	CurrentBodyGridPositions.Empty();

	const FIntPoint BackwardOffset = DirectionToGridOffset(CurrentDirection) * -1; // We want to add body segments in the opposite direction of the current movement, so we multiply the grid offset by -1 to get the backward offset.
	FIntPoint NextBodyCell = CurrentGridPosition + BackwardOffset;

	for (int32 i = 0; i < NumSegments; ++i)
	{
		CurrentBodyGridPositions.Add(NextBodyCell);
		NextBodyCell += BackwardOffset; // Move the next body cell further back for the next segment
	}
}

void ASnakePawn::TickFreeMovement(float DeltaTime)
{
	FVector MovementVector = GetVectorFromDirection(CurrentDirection);
	FVector DesiredOffset = MovementVector * MoveSpeed * DeltaTime;
	AddActorWorldOffset(DesiredOffset, true);
}

bool ASnakePawn::IsValidTurn(ESnakeDirection NewDirection) const
{
	if (NewDirection == CurrentDirection)
	{
		return false;
	}
	switch (CurrentDirection)
	{
	case ESnakeDirection::Up:
		return NewDirection != ESnakeDirection::Down;
	case ESnakeDirection::Down:
		return NewDirection != ESnakeDirection::Up;
	case ESnakeDirection::Left:
		return NewDirection != ESnakeDirection::Right;
	case ESnakeDirection::Right:
		return NewDirection != ESnakeDirection::Left;
	default:
		return false; // Should never happen, but we return false just in case
	}
}

void ASnakePawn::UpdateDirection(ESnakeDirection NewDirection)
{
	switch (NewDirection)
	{
		case ESnakeDirection::Up:	SetActorRotation(FRotator(0.f, 0.f, 0.f));
			break;
		case ESnakeDirection::Down:	SetActorRotation(FRotator(0.f, 180.f, 0.f));
			break;
		case ESnakeDirection::Left:	SetActorRotation(FRotator(0.f, -90.f, 0.f));
			break;
		case ESnakeDirection::Right:SetActorRotation(FRotator(0.f, 90.f, 0.f));
			break;
	}
}

void ASnakePawn::HandleDirectionChange()
{
	if (CurrentDirection != RequestedDirection && IsValidTurn(RequestedDirection))
	{
		CurrentDirection = RequestedDirection;
		UpdateDirection(CurrentDirection);
		UE_LOG(LogTemp, Warning, TEXT("Direction changed to: %s"), *UEnum::GetValueAsString(CurrentDirection));
	}
}

void ASnakePawn::DrawDebugInfo()
{
	if (bShowDebugCollision && CollisionSphere)
	{
		DrawDebugSphere(
			GetWorld(),
			CollisionSphere->GetComponentLocation(),
			CollisionSphere->GetScaledSphereRadius(),
			16,
			FColor::Green,
			false,
			-1.f, // Duration <0 means for one frame
			0,
			2.0f // Line thickness
		);
	}

	// Draw pawn's forward vector arrow (Blue)
	FVector ForwardStart = GetActorLocation();
	FVector ForwardEnd = ForwardStart + GetActorForwardVector() * 100.f; // 100 units in front
	DrawDebugDirectionalArrow(GetWorld(), ForwardStart, ForwardEnd, 50.f, FColor::Blue, false, -1.f, 0, 3.0f);

	// Draw pawn's right vector arrow (Red)
	FVector RightStart = GetActorLocation();
	FVector RightEnd = RightStart + GetActorRightVector() * 100.f; // 100 units to the right
	DrawDebugDirectionalArrow(GetWorld(), RightStart, RightEnd, 50.f, FColor::Red, false, -1.f, 0, 3.0f);
}

FIntPoint ASnakePawn::DirectionToGridOffset(ESnakeDirection Direction) const
{
	switch (Direction)
	{
	case ESnakeDirection::Up: return FIntPoint(1, 0);	// +X
	case ESnakeDirection::Down: return FIntPoint(-1, 0);// -X
	case ESnakeDirection::Left: return FIntPoint(0, -1);// -Y
	case ESnakeDirection::Right: return FIntPoint(0, 1);// +Y
	default: return FIntPoint(0, 0);
	}
}

FVector ASnakePawn::GridToWorldLocation(const FIntPoint& GridPosition) const
{
	if (GridManager)
	{
		return GridManager->GridToWorld(GridPosition);
	}
	return GridOrigin + FVector(GridPosition.X * CellSize, GridPosition.Y * CellSize, 0.f);
}

bool ASnakePawn::WouldHitWall(const FIntPoint& NextCell) const
{
	if (!GridManager) return false;
	// This assumes our border cells are walls.
	return GridManager->IsBlockedCell(NextCell); // IsWallCell?
	//return NextCell.X <= 0 || NextCell.X >= GridDimensions.X - 1
	//	|| NextCell.Y <= 0 || NextCell.Y >= GridDimensions.Y - 1;
}

bool ASnakePawn::WouldHitSelf(const FIntPoint& NextCell) const
{
	if (CurrentBodyGridPositions.Num() == 0)
	{
		return false;
	}

	// Special case:
	// Moving into the current tail cell is allowed if the tail will move away this step.
	const bool bTailWillStayThisStep = (PendingGrowth > 0);

	for (int32 i = 0; i < CurrentBodyGridPositions.Num(); ++i)
	{
		const bool bIsTail = (i == CurrentBodyGridPositions.Num() - 1);
		if (!bTailWillStayThisStep && bIsTail)
		{
			continue;
		}

		if (CurrentBodyGridPositions[i] == NextCell)
		{
			return true;
		}
	}

	return false;
}

void ASnakePawn::GrowSnake(int32 Amount)
{
	PendingGrowth += FMath::Max(Amount, 0); // Ensure we don't decrease the snake's length
}

void ASnakePawn::HandleSnakeDeath()
{
	if (bIsDead) return;
	bIsDead = true;
	OnSnakeDied.Broadcast();
	UE_LOG(LogTemp, Warning, TEXT("Snake has hit a wall and died!"));
}

void ASnakePawn::ResetSnake()
{
	const FIntPoint SpawnCell = GetClampedStartGridPosition();
	CurrentGridPosition = SpawnCell;
	PendingNextGridPosition = SpawnCell;

	CurrentDirection = ESnakeDirection::Right;
	RequestedDirection = ESnakeDirection::Right;
	UpdateDirection(CurrentDirection);
	
	const FVector ResetLocation = GridToWorldLocation(CurrentGridPosition);
	SetActorLocation(ResetLocation);

	StepStartWorldLocation = ResetLocation;
	StepTargetWorldLocation = ResetLocation;
	MoveInterpolationProgress = 0.f;
	bIsMovingToTarget = false;
	bIsDead = false;

	// Reset body segments
	CurrentBodyGridPositions.Empty();
	PreviousBodyGridPositions.Empty();
	ClearBodyVisuals();

	AddInitialBodySegments(InitialBodyLength);
	EnsureBodySegmentMeshCount();
	UpdateBodyVisuals(1.f); // We can call UpdateBodyVisuals with Alpha = 1 to immediately position the body segments at their correct locations based on the current grid positions, since we're resetting the snake and we want the body segments to appear in their correct positions right away without any interpolation.

	PendingGrowth = 0;
}

FIntPoint ASnakePawn::GetClampedStartGridPosition() const
{
	return FIntPoint(
		FMath::Clamp(StartGridPosition.X, 1, GridDimensions.X - 2),
		FMath::Clamp(StartGridPosition.Y, 1, GridDimensions.Y - 2)
	);
}

void ASnakePawn::HandleFoodOverlap(AFoodActor* FoodActor)
{
	if (bIsDead || !FoodActor)
	{
		return;
	}

	GrowSnake(1);
	FoodActor->DeactivateFood();
	
	OnFoodConsumed.Broadcast(10);
}

TArray<FIntPoint> ASnakePawn::GetAllOccupiedGridCells() const
{
	TArray<FIntPoint> Occupied = CurrentBodyGridPositions;
	Occupied.Insert(CurrentGridPosition, 0);
	return Occupied;
}

void ASnakePawn::CacheGridManager()
{
	GridManager = Cast<AGridManagerActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManagerActor::StaticClass()));
}

void ASnakePawn::ApplyStageSettings(float NewCellSize, FIntPoint NewGridDimensions, float NewMoveStepTime)
{
	CellSize = NewCellSize;
	GridDimensions = NewGridDimensions;
	MoveStepTime = NewMoveStepTime;
}

