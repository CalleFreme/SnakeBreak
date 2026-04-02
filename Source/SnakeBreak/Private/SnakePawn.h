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
	// Input callback functions
	void Input_Move(const FInputActionValue& Value);
	void Input_Turn(const FInputActionValue& Value);
	// (TO DO): use two separate functions instead of relying on Completed:
	// - Input_MoveStop()
	// - Input_TurnStop()

	// REAL TO DO: Implement proper Snake movement with Up, Down, Right, Left directions instead of free movement with forward and turn input. This will require a different input setup (e.g. four separate input actions for each direction, or using a 2D vector input and converting it to discrete directions in code), as well as a different movement implementation that moves the snake in a grid-based manner and handles turning at right angles instead of free rotation.

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
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float TurnSpeed = 90.f;

	// Current input values
	float CurrentForwardInput = 0.f;
	float CurrentTurnInput = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "100.0"))
	float CollisionSphereRadius = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowDebugCollision = true;
};
