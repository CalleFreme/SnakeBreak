#include "SnakePawn.h"
#include "FoodActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "SnakeGameMode.h"
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

	if (bShowDebugCollision)
	{
		DrawDebugInfo();
	}
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
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!Subsystem)
	{
		return;
	}

	// Important when restarting / re-possessing:
	// avoid stacking stale contexts.
	if (KeyboardInputMapping)
	{
		Subsystem->RemoveMappingContext(KeyboardInputMapping);
	}

	if (GamepadInputMapping)
	{
		Subsystem->RemoveMappingContext(GamepadInputMapping);
	}

	const int32 LocalPlayerIndex = LocalPlayer->GetControllerId();

	// For our supported setup:
	// Player 0 = keyboard
	// Player 1 = gamepad
	UInputMappingContext* MappingToUse = nullptr;

	if (PlayerSlotIndex == 0)
	{
		MappingToUse = KeyboardInputMapping;
	}
	else if (PlayerSlotIndex == 1)
	{
		MappingToUse = GamepadInputMapping;
	}

	if (MappingToUse)
	{
		Subsystem->AddMappingContext(MappingToUse, 0);

		UE_LOG(LogTemp, Warning,
			TEXT("Snake slot %d added input mapping %s for LocalPlayer ControllerId %d."),
			PlayerSlotIndex,
			*MappingToUse->GetName(),
			LocalPlayerIndex);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Snake slot %d has no input mapping assigned."),
			PlayerSlotIndex);
	}
}

// Called to bind functionality to input
void ASnakePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent)
	{
		return;
	}

	if (TurnKeyboardAction)
	{
		EnhancedInputComponent->BindAction(
			TurnKeyboardAction,
			ETriggerEvent::Triggered,
			this,
			&ASnakePawn::Input_TurnKeyboard);
	}

	if (TurnGamepadAction)
	{
		EnhancedInputComponent->BindAction(
			TurnGamepadAction,
			ETriggerEvent::Triggered,
			this,
			&ASnakePawn::Input_TurnGamepad);

		EnhancedInputComponent->BindAction(
			TurnGamepadAction,
			ETriggerEvent::Completed,
			this,
			&ASnakePawn::Input_TurnGamepadCompleted);
	}
}

void ASnakePawn::Input_TurnKeyboard(const FInputActionValue& Value)
{
	HandleTurnVector(Value.Get<FVector2D>());
}

void ASnakePawn::Input_TurnGamepad(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();

	constexpr float DeadZone = 0.5f;

	if (Input.SizeSquared() < DeadZone * DeadZone)
	{
		return;
	}

	if (!bGamepadStickTurnReady)
	{
		return;
	}

	if (HandleTurnVector(Input))
	{
		bGamepadStickTurnReady = false;
	}
}

void ASnakePawn::Input_TurnGamepadCompleted(const FInputActionValue& Value)
{
	bGamepadStickTurnReady = true;
}

void ASnakePawn::Input_TryTurnUp(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		RequestDirection(ESnakeDirection::Up);
	}
}

void ASnakePawn::Input_TryTurnDown(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		RequestDirection(ESnakeDirection::Down);
	}
}

void ASnakePawn::Input_TryTurnLeft(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		RequestDirection(ESnakeDirection::Left);
	}
}

void ASnakePawn::Input_TryTurnRight(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		RequestDirection(ESnakeDirection::Right);
	}
}

bool ASnakePawn::HandleTurnVector(const FVector2D& Input)
{
	if (Input.IsNearlyZero())
	{
		return false;
	}

	ESnakeDirection DesiredDirection;

	if (FMath::Abs(Input.X) > FMath::Abs(Input.Y))
	{
		DesiredDirection = Input.X > 0.f
			? ESnakeDirection::Right
			: ESnakeDirection::Left;
	}
	else
	{
		DesiredDirection = Input.Y > 0.f
			? ESnakeDirection::Up
			: ESnakeDirection::Down;
	}

	return RequestDirection(DesiredDirection);
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

	// if (WouldHitWall(PendingNextGridPosition) || WouldHitSelf(PendingNextGridPosition))
	// {
	// 	HandleSnakeDeath();
	// 	return;
	// }

	if (!IsNextGridCellSafe(PendingNextGridPosition))
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
		UE_LOG(LogTemp, Verbose, TEXT("Direction changed to: %s"), *UEnum::GetValueAsString(CurrentDirection));
	}
}

void ASnakePawn::DrawDebugInfo()
{
	if (!bShowDebugCollision)
	{
		return;
	}

	if (CollisionSphere)
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

FIntPoint ASnakePawn::DirectionToGridOffset(ESnakeDirection Direction)
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
	if (!GridManager)
	{
		return false;
	}
	
	return GridManager->IsBlockedCell(NextCell);
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
	if (bIsDead)
	{
		return;
	}
	
	bIsDead = true;
	OnSnakeDied.Broadcast(this); // THIS snake has died
	UE_LOG(LogTemp, Warning, TEXT("Snake has hit a wall and died!"));
}

void ASnakePawn::ResetSnake()
{
	const FIntPoint SpawnCell = GetClampedStartGridPosition();
	CurrentGridPosition = SpawnCell;
	PendingNextGridPosition = SpawnCell;

	CurrentDirection = StartDirection;
	RequestedDirection = StartDirection;
	UpdateDirection(CurrentDirection);

	const FVector ResetLocation = GridToWorldLocation(CurrentGridPosition);
	SetActorLocation(ResetLocation);

	StepStartWorldLocation = ResetLocation;
	StepTargetWorldLocation = ResetLocation;
	MoveInterpolationProgress = 0.f;
	bIsMovingToTarget = false;
	bIsDead = false;

	CurrentBodyGridPositions.Empty();
	PreviousBodyGridPositions.Empty();
	TargetBodyGridPositions.Empty();
	ClearBodyVisuals();

	AddInitialBodySegments(InitialBodyLength);
	EnsureBodySegmentMeshCount();
	UpdateBodyVisuals(1.f);

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

bool ASnakePawn::IsNextGridCellSafe(const FIntPoint& NextCell, bool bCheckOtherSnakes) const
{
	if (WouldHitWall(NextCell) || WouldHitSelf(NextCell))
	{
		return false;
	}

	if (bCheckOtherSnakes)
	{
		const ASnakeGameMode* SnakeGM = GetWorld()->GetAuthGameMode<ASnakeGameMode>();
		if (SnakeGM && SnakeGM->IsCellOccupiedByOtherSnake(this, NextCell))
		{
			return false;
		}
	}

	return true;
}

void ASnakePawn::GetProjectedOccupiedGridCellsAfterMove(
	const FIntPoint& NextCell,
	TSet<FIntPoint>& OutOccupiedCells) const
{
	OutOccupiedCells.Reset();
	OutOccupiedCells.Add(NextCell);

	if (CurrentBodyGridPositions.Num() == 0 && PendingGrowth <= 0)
	{
		return;
	}

	// After a legal move the old head becomes the first body segment.
	OutOccupiedCells.Add(CurrentGridPosition);

	const bool bTailWillStayThisStep = PendingGrowth > 0;
	const int32 BodyCellsToKeep = bTailWillStayThisStep
		? CurrentBodyGridPositions.Num()
		: CurrentBodyGridPositions.Num() - 1;

	for (int32 i = 0; i < BodyCellsToKeep; ++i)
	{
		OutOccupiedCells.Add(CurrentBodyGridPositions[i]);
	}
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

void ASnakePawn::SetStartGridPosition(FIntPoint NewStartGridPosition)
{
	StartGridPosition = NewStartGridPosition;
}

bool ASnakePawn::RequestDirection(ESnakeDirection NewDirection)
{
	if (!CanRequestDirection(NewDirection))
	{
		return false;
	}

	RequestedDirection = NewDirection;
	return true;
}

bool ASnakePawn::CanRequestDirection(ESnakeDirection NewDirection) const
{
	if (bIsDead)
	{
		return false;
	}

	if (NewDirection == CurrentDirection)
	{
		return true;
	}

	return IsValidTurn(NewDirection);
}

FIntPoint ASnakePawn::GetNextCellForDirection(ESnakeDirection Direction) const
{
	return CurrentGridPosition + DirectionToGridOffset(Direction);
}

void ASnakePawn::SetInitialDirection(ESnakeDirection NewDirection)
{
	StartDirection = NewDirection;
	CurrentDirection = NewDirection;
	RequestedDirection = NewDirection;
	UpdateDirection(NewDirection);
}

void ASnakePawn::SetPlayerSlotIndex(int32 NewPlayerSlotIndex)
{
	PlayerSlotIndex = NewPlayerSlotIndex;
}
