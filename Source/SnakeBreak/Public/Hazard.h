// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridMovementComponent.h"
#include "Hazard.generated.h"

UENUM(BlueprintType)
enum class EHazardType : uint8 
{
	MovingWall,
	LaserWall,
	MovingLaserBeam
};

UCLASS(Abstract)
class SNAKEBREAK_API AHazard : public AActor
{
	GENERATED_BODY()

public:
	AHazard();

	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
	EHazardType HazardType;
	
	UFUNCTION(BlueprintCallable, Category = "Hazard")
	virtual void EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex);

	UFUNCTION(BlueprintCallable, Category = "Hazard")
	bool IsHazardActive() const { return bHazardActive; }

	UFUNCTION(BlueprintCallable, Category = "Hazard")
	bool HasAssignedVisualMesh() const;

	UFUNCTION(BlueprintCallable, Category = "Hazard")
	virtual void SetHazardActive(bool bNewActive);

	static constexpr int32 HeadHitIndex = -1;

protected:
	void ConfigureHazardCollision(class UPrimitiveComponent* CollisionComponent) const;
	void EliminateTargetActor(AActor* TargetActor);
	void TrimTargetTail(AActor* TargetActor, int32 HitSegmentIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> HazardMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard")
	bool bHazardActive = true;
};




// LASER WALL HAZARD
UCLASS()
class SNAKEBREAK_API ALaserWall : public AHazard
{
	GENERATED_BODY()
	
public:
	ALaserWall();
	
	virtual void BeginPlay() override;
	virtual void EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex) override;
	virtual void SetHazardActive(bool bNewActive) override;
	
	void InitializeLaser(float NewToggleRateSeconds);
	void ToggleLaser();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Settings", meta = (ClampMin = "0.0"))
	float ToggleRateSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Settings")
	bool bStartsActive = true;
	
protected:
	void ConfigureLaserTimer();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Laser Settings")
	TObjectPtr<class UBoxComponent> LaserCollision;

	FTimerHandle ToggleTimerHandle;
};




// MOVING LASER BEAM
UCLASS()
class SNAKEBREAK_API AMovingLaserBeam : public AHazard
{
	GENERATED_BODY()
	
public:
	AMovingLaserBeam();

	virtual void BeginPlay() override;
	virtual void EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex) override;
		
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UGridMovementComponent> GridMovementComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Laser Settings")
	TObjectPtr<class UBoxComponent> LaserCollision;
};




// MOVING WALL HAZARD
UCLASS()
class SNAKEBREAK_API AMovingWall : public AHazard
{
	GENERATED_BODY()
 
public:
	AMovingWall();
 
	virtual void BeginPlay() override;
	virtual void EliminateTarget(AActor* TargetActor, int32 HitSegmentIndex) override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UGridMovementComponent> GridMovementComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<class UBoxComponent> WallCollision;
};
