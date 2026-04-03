#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "DrawDebugHelpers.h"
#include "SnakePawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UStaticMeshComponent;
class USpringArmComponent;
class USphereComponent;

// We can create an enum type to let the Snake keep track of its requested movement direction, which will be useful for implementing proper Snake movement in the future. For now, we can just use the forward and turn input for free movement, but in the future we can switch to a grid-based movement system where the snake moves in discrete steps in one of four directions (up, down, left, right) and turns at right angles. This will require a different input setup (e.g. four separate input actions for each direction, or using a 2D vector input and converting it to discrete directions in code), as well as a different movement implementation that moves the snake in a grid-based manner and handles turning at right angles instead of free rotation. Where do you put the enum code? You can put it either in the header file (SnakePawn.h) or in the source file (SnakePawn.cpp), depending on how you want to use it. If you want to use the enum in other classes or Blueprints, it's better to put it in the header file so it's more accessible. If it's only used internally within the SnakePawn class, you can put it in the source file to keep it more encapsulated. For this simple game, we can just put it in the header file for now, since we might want to use it in Blueprints later when we implement the proper Snake movement.

struct GridPosition
{
	int32 X;
	int32 Y;
	GridPosition(int32 InX, int32 InY) : X(InX), Y(InY) {}
};

UENUM(BlueprintType)
enum class ESnakeDirection : uint8
{
	Up,
	Down,
	Left,
	Right
};

UCLASS()
class ASnakePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASnakePawn();


protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	ESnakeDirection CurrentDirection = ESnakeDirection::Right;
	ESnakeDirection RequestedDirection = ESnakeDirection::Right;
	// We keep track of current grid position.
	GridPosition CurrentGridPosition = GridPosition(0, 0);

	// Keep track of target world position for smooth movement:
	FVector TargetWorldPosition = FVector::ZeroVector;

	// Input callback functions
	void Input_TryTurnUp(const FInputActionValue& Value);
	void Input_TryTurnDown(const FInputActionValue& Value);
	void Input_TryTurnLeft(const FInputActionValue& Value);
	void Input_TryTurnRight(const FInputActionValue& Value);

	void UpdateDirection(ESnakeDirection NewDirection);
	void MoveForward(float DeltaTime);
	void TickGridMovement(float DeltaTime);
	void TickFreeMovement(float DeltaTime);
	bool IsValidTurn(ESnakeDirection NewDirection) const;

	// VisibleAnywhere = can be seen in the editor, but not modified; BlueprintReadOnly = can be read in Blueprints, but not modified; Category = how it is grouped in the editor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	// AllowPrivateAccess = true means that even though this variable is private in C++, it can still be accessed and modified in Blueprints. This is useful for components that we want to set up in C++ but still allow designers to tweak them in the editor without needing to modify the C++ code. In this case, we want to be able to adjust the collision sphere's properties (like radius) in the editor, so we set AllowPrivateAccess to true.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	// EditAnywhere = can be set per instance in the level, as well as in the Blueprint defaults
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnUpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnDownAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnLeftAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnRightAction;

	// Could use a movement mode enum instead
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bUseGridMovement = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float TurnSpeed = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "100.0"))
	float CollisionSphereRadius = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowDebugCollision = true;
};
