#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "SnakePawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UStaticMeshComponent;
class USpringArmComponent;
class USphereComponent;

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
	ASnakePawn();


protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	float CellSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "5", ClampMax = "100"))
	FIntPoint GridDimensions = FIntPoint(20, 20);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float MoveStepTime = 0.2f; // Time it takes to move from one grid cell to the next when using grid movement.

private:
	ESnakeDirection CurrentDirection = ESnakeDirection::Right;
	ESnakeDirection RequestedDirection = ESnakeDirection::Right;
	FIntPoint CurrentGridPosition = FIntPoint(0, 0);
	FIntPoint PendingNextGridPosition = FIntPoint(0, 0);

	FVector StepStartWorldLocation = FVector::ZeroVector;
	FVector StepTargetWorldLocation = FVector::ZeroVector;

	float MoveInterpolationProgress = 0.f;
	bool bIsMovingToTarget = false;
	bool bIsDead = false;
	FTimerHandle ResetTimerHandle;

	FVector GetVectorFromDirection(ESnakeDirection Direction) const;
	FIntPoint DirectionToGridOffset(ESnakeDirection Direction) const;
	FVector GridToWorldLocation(const FIntPoint& GridPosition) const;

	// Input callback functions
	void Input_TryTurnUp(const FInputActionValue& Value);
	void Input_TryTurnDown(const FInputActionValue& Value);
	void Input_TryTurnLeft(const FInputActionValue& Value);
	void Input_TryTurnRight(const FInputActionValue& Value);

	void HandleDirectionChange();
	void UpdateDirection(ESnakeDirection NewDirection);
	void TickGridMovement(float DeltaTime);
	void TickFreeMovement(float DeltaTime);
	bool IsValidTurn(ESnakeDirection NewDirection) const;
	void DrawDebugInfo();
	FIntPoint GetClampedStartGridPosition() const;

	bool WouldHitWall(const FIntPoint& NextCell) const;
	void HandleSnakeDeath();
	void ResetSnake();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BoxMesh;

	// AllowPrivateAccess = true means that even though this variable is private in C++, it can still be accessed and modified in Blueprints.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	FIntPoint StartGridPosition = FIntPoint(10, 10);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	FVector GridOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MoveSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float TurnSpeed = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "100.0"))
	float CollisionSphereRadius = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowDebugCollision = true;
};
