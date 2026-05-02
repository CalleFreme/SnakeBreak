// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridMovementComponent.generated.h"

class AGridManagerActor;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SNAKEBREAK_API UGridMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridMovementComponent();

	
	void SetTargetLocation(FVector NewLocation);

	void SetMovementSpeed(float NewMovementSpeed);

	UFUNCTION(BlueprintCallable, Category = "GridMovement")
	void MoveInGridDirection(const FVector& Direction);
	
	void SetPatrolPoints(const TArray<FIntPoint>& GridPoints);
	const TArray<FIntPoint>& GetPatrolPoints() const { return PatrolPoints; }
	void ConfigurePatrol(
		const TArray<FIntPoint>& GridPoints,
		float NewMovementSpeed,
		float NewPatrolWaitSeconds,
		bool bShouldAutoStart);
	void StartPatrol();
	void StopPatrol();
	void MoveToNextPatrolPoint();
	
	UPROPERTY(EditAnywhere, Category = "GridMovement")
	float GridSize;

	UPROPERTY(EditAnywhere, Category = "GridMovement", meta = (ClampMin = "1.0"))
	float MovementSpeed = 100.f;

	UPROPERTY(EditAnywhere, Category = "GridMovement", meta = (ClampMin = "0.1"))
	float AcceptanceDistance = 1.f;

	UPROPERTY(EditAnywhere, Category = "GridMovement", meta = (ClampMin = "0.0"))
	float PatrolWaitSeconds = 0.f;

	UPROPERTY(EditAnywhere, Category = "GridMovement")
	bool bAutoStartPatrol = true;
	
	int32 CurrentTargetIndex;
	
	UPROPERTY(EditAnywhere, Category = "GridMovement")
	FVector CurrentTargetLocation = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GridMovement")
	FVector CurrentDirection = FVector::ForwardVector;
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category = "GridMovement")
	TArray<FIntPoint> PatrolPoints;
	
	TObjectPtr<AGridManagerActor> GridManager;

	bool bIsMovingToTarget = false;
	bool bPatrolActive = false;
	FTimerHandle PatrolTimerHandle;

	void CacheGridManager();
	void SnapToCurrentPatrolPoint();
	void ScheduleNextPatrolMove();
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
