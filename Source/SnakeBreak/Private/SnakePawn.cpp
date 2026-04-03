#include "SnakePawn.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SphereComponent.h"

// Sets default values
ASnakePawn::ASnakePawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics; // Ensures this actor tick AFTER all physics and movement has been resolved. This is important for our snake pawn, because we want to update the snake's position and direction based on player input after all movement and collision has been processed for the frame, so that the snake's movement feels responsive and accurate to the player's input without being affected by physics or collision issues.

	// Automatically possess this pawn with the first player controller
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	// Alternatively, you would set up GameMode to spawn this pawn class and possess it, but for a simple game like this, AutoPossessPlayer is easier

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(CollisionSphereRadius);
	CollisionSphere->SetCollisionProfileName(TEXT("Pawn")); // Set the collision profile to "Pawn", which is a predefined profile that allows it to collide with the world and other pawns, but not block the camera or other things. This is important for our snake pawn, because we want it to be able to collide with the ground and other objects, but we don't want it to block the camera or cause weird physics interactions with the box mesh.

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetupAttachment(RootComponent);
	BoxMesh->SetRelativeLocation(FVector::ZeroVector); // Center the mesh on the root component (collision sphere)
	// We don't want the mesh to collide with the ground or other objects, because we're going to use a separate collision sphere for that, and we want the mesh to just be a visual representation of the snake's head without affecting physics or collisions. So we set it to NoCollision, which means it won't generate any collision events or block anything. We also set SimulatePhysics to false, because we don't want the mesh to be affected by physics forces or gravity, since we're going to move it manually in the Tick function based on player input:
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxMesh->SetSimulatePhysics(false);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.f;
	SpringArm->bUsePawnControlRotation = false; // fixed camera, not controller-driven
	SpringArm->SetUsingAbsoluteRotation(true);  // fixed world rotation, not relative to pawn rotation
	SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	bUseControllerRotationYaw = false;
}

// Called when the game starts or when spawned
void ASnakePawn::BeginPlay()
{
	Super::BeginPlay();

	RequestedDirection = CurrentDirection;
	UpdateDirection(CurrentDirection);	

	if (bUseGridMovement)
	{
		SetActorLocation(GridToWorldLocation(CurrentGridPosition));
		StepStartWorldLocation = GetActorLocation();
		StepTargetWorldLocation = GetActorLocation();
		PendingNextGridPosition = CurrentGridPosition;
		MoveInterpolationProgress = 0.f;
		bIsMovingToTarget = false;
	}
	else
	{
		FVector StartLocation = GetActorLocation();
		StartLocation.Z += 15.f;
		SetActorLocation(StartLocation);
	}

	// Why Cast? Because GetController() returns an AController*, but we need to check if it's an APlayerController* in order to access the local player and enhanced input subsystem. We also check if the cast is successful, because in some cases (e.g. if this pawn is possessed by an AI controller), there might not be a player controller, and we don't want to crash if that's the case.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			// Better name for this variable might be "EnhancedInputSubsystem" or "PlayerEnhancedInputSubsystem", but "Subsystem" is fine for this small scope. We also check if the subsystem is valid, because if we're using the Enhanced Input system, it should be there, but we want to avoid crashing if it's not for some reason (e.g. if the player controller or local player is set up in a way that doesn't include the subsystem).
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

// Called every frame
void ASnakePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


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

// Called to bind functionality to input
void ASnakePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// We check:
	// 1. That the PlayerInputComponent is a UEnhancedInputComponent, which it should be if we're using the Enhanced Input system; and
	// 2. That the MoveAction and TurnAction are set, to avoid binding to null actions
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
		HandleDirectionChange();

		const FIntPoint GridOffset = DirectionToGridOffset(CurrentDirection);
		PendingNextGridPosition = CurrentGridPosition + GridOffset;

		StepStartWorldLocation = GridToWorldLocation(CurrentGridPosition);
		StepTargetWorldLocation = GridToWorldLocation(PendingNextGridPosition);
		MoveInterpolationProgress = 0.f; // Since we're just starting to move towards the new target, we reset the interpolation progress to 0
		bIsMovingToTarget = true;
	}

	MoveInterpolationProgress += DeltaTime / MoveStepTime;
	const float Alpha = FMath::Clamp(MoveInterpolationProgress, 0.f, 1.f);

	// Use Lerp for constant speed across the cell
	FVector NewLocation = FMath::Lerp(StepStartWorldLocation, StepTargetWorldLocation, Alpha);
	SetActorLocation(NewLocation);

	if (Alpha >= 1.f)
	{
		// We've reached the target grid location
		CurrentGridPosition = PendingNextGridPosition;
		SetActorLocation(StepTargetWorldLocation); // Ensure we snap exactly to the target location
		bIsMovingToTarget = false; // We're now at the target, so we can start the process again on the next tick
	}
}

void ASnakePawn::TickFreeMovement(float DeltaTime)
{
	// Is this going to work? 
	FVector MovementVector = GetVectorFromDirection(CurrentDirection);
	FVector DesiredOffset = MovementVector * MoveSpeed * DeltaTime;
	AddActorWorldOffset(DesiredOffset, true);
}

bool ASnakePawn::IsValidTurn(ESnakeDirection NewDirection) const
{
	// A turn is valid if it's not the same as the current direction, and it's not directly opposite to the current direction (e.g. if we're moving up, we can't turn down)
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
	return GridOrigin + FVector(GridPosition.X * CellSize, GridPosition.Y * CellSize, 0.f);
}