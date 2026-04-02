// Fill out your copyright notice in the Description page of Project Settings.


#include "SnakePawn.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"

// Sets default values
ASnakePawn::ASnakePawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Automatically possess this pawn with the first player controller
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	// Alternatively, you would set up GameMode to spawn this pawn class and possess it, but for a simple game like this, AutoPossessPlayer is easier

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(CollisionSphereRadius);
	RootComponent = CollisionSphere;
	CollisionSphere->SetCollisionProfileName(TEXT("Pawn")); // Set the collision profile to "Pawn", which is a predefined profile that allows it to collide with the world and other pawns, but not block the camera or other things. This is important for our snake pawn, because we want it to be able to collide with the ground and other objects, but we don't want it to block the camera or cause weird physics interactions with the box mesh.

	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetupAttachment(RootComponent);
	// We don't want the mesh to collide with the ground or other objects, because we're going to use a separate collision sphere for that, and we want the mesh to just be a visual representation of the snake's head without affecting physics or collisions. So we set it to NoCollision, which means it won't generate any collision events or block anything. We also set SimulatePhysics to false, because we don't want the mesh to be affected by physics forces or gravity, since we're going to move it manually in the Tick function based on player input:
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxMesh->SetSimulatePhysics(false);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.f;
	SpringArm->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));
	// SpringArm->bUsePawnControlRotation = true; // This would make the camera rotate with the controller/pawn, but we want a fixed camera angle for this game
	SpringArm->bDoCollisionTest = false; // Disable collision testing so the camera doesn't get pushed in when it hits something

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName); // Attach the camera to the end of the spring arm, SocketName is just a predefined socket at the end of the spring arm
}

// Called when the game starts or when spawned
void ASnakePawn::BeginPlay()
{
	Super::BeginPlay();

	// Bump up the Z slightly to avoid initial overlap/floor stuck
	FVector StartLocation = GetActorLocation();
	StartLocation.Z += 30.f;
	SetActorLocation(StartLocation);

	// Why Cast? Because GetController() returns an AController*, but we need to check if it's an APlayerController* in order to access the local player and enhanced input subsystem. We also check if the cast is successful, because in some cases (e.g. if this pawn is possessed by an AI controller), there might not be a player controller, and we don't want to crash if that's the case.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Why Cast? Because GetLocalPlayer() is a function of ULocalPlayer, which is a subclass of UPlayer, which is what GetController()->GetLocalPlayer() returns. So we need to cast to ULocalPlayer to access that function. We also check if the cast is successful, because in some cases (e.g. if this pawn is possessed by an AI controller), there might not be a local player associated with the controller, and we don't want to crash if that's the case.
		if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PC->GetLocalPlayer()))
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
	// Apply Forward Movement
	if (!FMath::IsNearlyZero(CurrentForwardInput))
	{
		UE_LOG(LogTemp, Warning, TEXT("Move: %.2f"), CurrentForwardInput);
		FVector DesiredOffset = GetActorForwardVector() * CurrentForwardInput * MoveSpeed * DeltaTime;
		AddActorWorldOffset(DesiredOffset, true);
	}

	// Apply Rotation
	if (!FMath::IsNearlyZero(CurrentTurnInput))
	{
		UE_LOG(LogTemp, Warning, TEXT("Turn: %.2f"), CurrentTurnInput);
		// Unreal uses left-handed rotation, so positive yaw rotates to the right, and negative yaw rotates to the left. This is opposite of what we might expect if we think in terms of a standard Cartesian coordinate system, but it's just something to keep in mind when setting up input bindings and interpreting input values.
		FRotator DesiredRotation = FRotator(0.f, CurrentTurnInput * TurnSpeed * DeltaTime, 0.f); // Rotate around the Z axis (yaw) based on the turn input, turn speed, and delta time. We multiply by delta time to make the rotation frame rate independent, so it rotates at the same speed regardless of frame rate.
		AddActorWorldRotation(DesiredRotation);
	}

	// IMPORTANT: Reset input if your Input Action is set to "Pressed" 
	// or keep it if using "Triggered" with a continuous axis.
	// For BoxRover, usually we reset or let the Input Action Zero it out.
	//CurrentForwardInput = 0.f;
	//CurrentTurnInput = 0.f;
}

// Called to bind functionality to input
void ASnakePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// We check:
	// 1. That the PlayerInputComponent is a UEnhancedInputComponent, which it should be if we're using the Enhanced Input system; and
	// 2. That the MoveAction and TurnAction are set, to avoid binding to null actions
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASnakePawn::Input_Move);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASnakePawn::Input_Move); // We bind to both Triggered and Completed so that we can start moving when the input is triggered, and stop moving when the input is completed (e.g. when the player releases the key)
		}
		if (TurnAction)
		{
			EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &ASnakePawn::Input_Turn);
			EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Completed, this, &ASnakePawn::Input_Turn);
		}
	}

}

void ASnakePawn::Input_Move(const FInputActionValue& Value)
{
	// We expect MoveAction to be a 1D axis (e.g. bound to W/S or Up/Down keys, or a gamepad trigger), so we get the Axis1D value from the input
	CurrentForwardInput = Value.Get<FInputActionValue::Axis1D>();
}

void ASnakePawn::Input_Turn(const FInputActionValue& Value)
{
	// We expect TurnAction to be a 1D axis (e.g. bound to A/D or Left/Right keys, or a gamepad stick), so we get the Axis1D value from the input
	CurrentTurnInput = Value.Get<FInputActionValue::Axis1D>(); // Could also take as float directly since we know it's an Axis1D, but using FInputActionValue::Axis1D is more explicit and allows for easier changes later if we want to use a different input type
}

