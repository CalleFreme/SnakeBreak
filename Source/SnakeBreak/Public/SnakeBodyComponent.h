#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SnakeBodyComponent.generated.h"

class AGridManagerActor;
class UMaterialInterface;
class UPrimitiveComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USphereComponent;
struct FHitResult;

UCLASS(ClassGroup=(Snake), meta=(BlueprintSpawnableComponent))
class SNAKEBREAK_API USnakeBodyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USnakeBodyComponent();

	void SetGridContext(AGridManagerActor* NewGridManager, float NewCellSize, const FVector& NewGridOrigin);
	void ResetBody(int32 NumSegments, const FIntPoint& HeadCell, const FIntPoint& BackwardOffset, float Alpha);
	void PrepareMoveTargets(const FIntPoint& CurrentHeadCell);
	void FinishMoveStep(const FIntPoint& OldHeadCell);
	void UpdateSegments(float Alpha);
	void ClearBody();
	void Grow(int32 Amount = 1);
	void TrimTail(int32 HitSegmentIndex);
	void DrawDebugBody() const;

	bool WouldHitSelf(const FIntPoint& NextCell) const;
	void GetProjectedOccupiedCellsAfterMove(
		const FIntPoint& CurrentHeadCell,
		const FIntPoint& NextCell,
		TSet<FIntPoint>& OutOccupiedCells) const;

	const TArray<FIntPoint>& GetBodyGridPositions() const { return CurrentBodyGridPositions; }
	int32 GetPendingGrowth() const { return PendingGrowth; }

private:
	UFUNCTION()
	void HandleBodySegmentOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	FVector GridToWorldLocation(const FIntPoint& GridPosition) const;
	void AddInitialBodySegments(int32 NumSegments, const FIntPoint& HeadCell, const FIntPoint& BackwardOffset);
	void UpdateBodyVisuals(float Alpha);
	void UpdateBodySegmentColliders(float Alpha);
	void EnsureBodySegmentMeshCount();
	void EnsureBodySegmentColliderCount();
	USphereComponent* CreateBodySegmentCollider(int32 SegmentIndex, const FIntPoint& GridCell);
	void ClearBodyVisuals();
	void ClearBodySegmentColliders();

	UPROPERTY()
	TObjectPtr<AGridManagerActor> GridManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	TArray<FIntPoint> CurrentBodyGridPositions;

	UPROPERTY()
	TArray<FIntPoint> PreviousBodyGridPositions;

	UPROPERTY()
	TArray<FIntPoint> TargetBodyGridPositions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	int32 PendingGrowth = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> BodySegmentMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<USphereComponent>> BodySegmentColliders;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> BodySegmentMeshAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> BodySegmentMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	FVector BodySegmentMeshScale = FVector(0.9f, 0.9f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true"))
	float BodySegmentMeshZOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Snake|Body", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float BodySegmentColliderRadius = 40.f;

	float CellSize = 100.f;
	FVector GridOrigin = FVector::ZeroVector;
};
