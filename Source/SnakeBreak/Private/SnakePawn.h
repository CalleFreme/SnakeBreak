#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "GridManagerActor.h"
#include "HazardTarget.h"
#include "SnakePawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UStaticMeshComponent;
class USpringArmComponent;
class USphereComponent;
class AFoodActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFoodConsumed, ASnakePawn*, EatingSnake, int32, ScoreValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSnakeDied, ASnakePawn*, DeadSnake);

UENUM(BlueprintType)
enum class ESnakeDirection : uint8
{
	Up,
	Down,
	Left,
	Right
};

UCLASS()
class ASnakePawn : public APawn, public IHazardTarget
{
	GENERATED_BODY()

public:
	ASnakePawn();

	UFUNCTION()
	void HandleFoodOverlap(AFoodActor* FoodActor);

	UFUNCTION()
	void HandleHeadOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
	UFUNCTION()
	void HandleBodySegmentOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);	

	UPROPERTY(BlueprintAssignable, Category = "Snake|Events")
	FOnFoodConsumed OnFoodConsumed;

	UPROPERTY(BlueprintAssignable, Category = "Snake|Events")
	FOnSnakeDied OnSnakeDied;

	UFUNCTION(BlueprintPure, Category = "Snake")
	TArray<FIntPoint> GetAllOccupiedGridCells() const;

	bool IsNextGridCellSafe(const FIntPoint& NextCell, bool bCheckOtherSnakes = true) const;
	void GetProjectedOccupiedGridCellsAfterMove(const FIntPoint& NextCell, TSet<FIntPoint>& OutOccupiedCells) const;

	void ResetSnake();
	void Eliminate();
	void TrimTail(int32 HitSegmentIndex);

	virtual void EliminateByHazard_Implementation(AHazard* Hazard) override;
	virtual void TrimTailByHazard_Implementation(AHazard* Hazard, int32 HitSegmentIndex) override;
	
	void CacheGridManager();
	
	void ApplyStageSettings(float NewCellSize, FIntPoint NewGridDimensions, float NewMoveStepTime);
	
	UFUNCTION(BlueprintCallable, Category = "Snake|Grid")
	void SetStartGridPosition(FIntPoint NewStartGridPosition);
	
	UFUNCTION(BlueprintCallable, Category = "Snake|Movement")
	bool RequestDirection(ESnakeDirection NewDirection);
	
	UFUNCTION(BlueprintPure, Category = "Snake|Movement")
	ESnakeDirection GetCurrentDirection() const { return CurrentDirection; }

	UFUNCTION(BlueprintPure, Category = "Snake|Movement")
	ESnakeDirection GetRequestedDirection() const { return RequestedDirection; }

	UFUNCTION(BlueprintPure, Category = "Snake|Grid")
	FIntPoint GetCurrentGridPosition() const { return CurrentGridPosition; }

	UFUNCTION(BlueprintPure, Category = "Snake|State")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Snake|Movement")
	bool IsMovingToTarget() const { return bIsMovingToTarget; }

	UFUNCTION(BlueprintPure, Category = "Snake|Movement")
	float GetMoveStepTime() const { return MoveStepTime; }

	UFUNCTION(BlueprintPure, Category = "Snake|Grid")
	FIntPoint GetNextCellForDirection(ESnakeDirection Direction) const;
	
	void SetInitialDirection(ESnakeDirection NewDirection);
	
	UFUNCTION(BlueprintPure, Category = "Snake|Movement")
	bool CanRequestDirection(ESnakeDirection NewDirection) const;
	
	static FIntPoint DirectionToGridOffset(ESnakeDirection Direction);
	
	UFUNCTION(BlueprintCallable, Category = "Snake|Player")
	void SetPlayerSlotIndex(int32 NewPlayerSlotIndex);
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void SetupEnhancedInput();
	
	UPROPERTY()
	bool bGamepadStickTurnReady = true;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	float CellSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "5", ClampMax = "100"))
	FIntPoint GridDimensions = FIntPoint(20, 20);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float MoveStepTime = 0.2f; // Time it takes to move from one grid cell to the next when using grid movement.

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake|Player", meta = (AllowPrivateAccess = "true"))
	int32 PlayerSlotIndex = INDEX_NONE;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake", meta = (AllowPrivateAccess = "true"))
	TArray<FIntPoint> CurrentBodyGridPositions;
	
	UPROPERTY()
	TObjectPtr<AGridManagerActor> GridManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake", meta = (AllowPrivateAccess = "true"))
	int32 PendingGrowth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snake|Movement", meta = (AllowPrivateAccess = "true"))
	ESnakeDirection StartDirection = ESnakeDirection::Right;
	
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

	// Snapshot of body before starting a step. So body segements can smoothly follow.
	TArray<FIntPoint> PreviousBodyGridPositions;
	TArray<FIntPoint> TargetBodyGridPositions;

	// Transient means this variable won't be saved/loaded or replicated. We only need it at runtime to keep track of the body segment meshes. In simple terms, it tells Unreal Engine not to worry about this variable when saving the game or sending data over the network, since it's only relevant while the game is running.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> BodySegmentMeshes;

	// Helpers
	FVector GetVectorFromDirection(ESnakeDirection Direction) const;
	FVector GridToWorldLocation(const FIntPoint& GridPosition) const;

	// Input callback functions
	void Input_TurnKeyboard(const FInputActionValue& Value);
	void Input_TurnGamepad(const FInputActionValue& Value);
	void Input_TurnGamepadCompleted(const FInputActionValue& Value);
	void Input_TryTurnUp(const FInputActionValue& Value);
	void Input_TryTurnDown(const FInputActionValue& Value);
	void Input_TryTurnLeft(const FInputActionValue& Value);
	void Input_TryTurnRight(const FInputActionValue& Value);

	bool HandleTurnVector(const FVector2D& Input);
	void HandleDirectionChange();
	void UpdateDirection(ESnakeDirection NewDirection);
	void TickGridMovement(float DeltaTime);
	void TickFreeMovement(float DeltaTime);
	bool IsValidTurn(ESnakeDirection NewDirection) const;
	void DrawDebugInfo();
	FIntPoint GetClampedStartGridPosition() const;

	bool WouldHitWall(const FIntPoint& NextCell) const;
	bool WouldHitSelf(const FIntPoint& NextCell) const;

	void HandleSnakeDeath();

	// Body system
	void StartNewMoveStep();
	void FinishMoveStep();
	void EnsureBodySegmentMeshCount(); // Need a way to make sure we have the right number of body segment meshes to match the body segments in CurrentBodyGridPositions. This function adds or removes meshes as needed.
	void EnsureBodySegmentColliderCount();
	void ClearBodyVisuals();
	void ClearBodySegmentColliders();
	void AddInitialBodySegments(int32 NumSegments);
	void UpdateBodySegments(float Alpha);
	void UpdateBodyVisuals(float Alpha); // Alpha is 0 to 1 representing progress from current grid cell to next grid cell
	void UpdateBodySegmentColliders(float Alpha);
	USphereComponent* CreateBodySegmentCollider(int32 SegmentIndex, const FIntPoint& GridCell);
	void GrowSnake(int32 Amount = 1);

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> SnakeHeadMesh;
		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	// Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> KeyboardInputMapping;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> GamepadInputMapping;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnKeyboardAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnGamepadAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnUpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnDownAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnLeftAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> TurnRightAction;

	// Movement / Grid
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bUseGridMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	FIntPoint StartGridPosition = FIntPoint(10, 10);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	FVector GridOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MoveSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "100.0"))
	float CollisionSphereRadius = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowDebugCollision = false;

	// Body visuals settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> BodySegmentMeshAsset = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<USphereComponent>> BodySegmentColliders;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> BodySegmentMaterial = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake", meta = (AllowPrivateAccess = "true"))
	int32 InitialBodyLength = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	FVector BodySegmentMeshScale = FVector(0.9f, 0.9f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	float BodySegmentMeshZOffset = 0.f; // How much to offset the body segment meshes on the Z axis, to prevent z-fighting with the head mesh and ground.


};
