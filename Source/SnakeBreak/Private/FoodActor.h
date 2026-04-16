// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoodActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ASnakePawn;

UCLASS()
class AFoodActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFoodActor();

	UFUNCTION(BlueprintCallable, Category = "Food")
	void SetFoodGridPosition(const FIntPoint& NewGridPosition, const FVector& NewWorldLocation);

	UFUNCTION(BlueprintPure, Category = "Food")
	FIntPoint GetFoodGridPosition() const { return FoodGridPosition; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleFoodOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherCamp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess))
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess))
	TObjectPtr<UStaticMeshComponent> FoodMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Food", meta = (AllowPrivateAccess))
	FIntPoint FoodGridPosition = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess))
	float CollisionRadius = 45.f;
};
