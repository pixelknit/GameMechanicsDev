// DecalInstanceProjector.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "DecalInstanceProjector.generated.h"

UCLASS()
class DR_TEST_API ADecalInstanceProjector : public AActor
{
    GENERATED_BODY()
    
public:    
    ADecalInstanceProjector();

protected:
    virtual void BeginPlay() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostEditMove(bool bFinished) override;
#endif
    
    // Box component for overlap detection
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* ProjectorBox;
    
    // Actor class to spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    TSubclassOf<AActor> ActorToSpawn;
    
    // Alternatively, spawn a simple static mesh actor with this mesh NOT WORKING AT THE MOMENT
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    UStaticMesh* StaticMeshToSpawn;
    
    // Material to apply to spawned static meshes (optional)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    UMaterialInterface* SpawnedMeshMaterial;
    
    // Number of spawn attempts per overlapping actor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "1", ClampMax = "1000"))
    int32 SpawnDensity = 50;
    
    // Random scale range
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomization")
    FVector2D ScaleRange = FVector2D(0.5f, 1.5f);
    
    // Random rotation range (in degrees)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomization")
    FVector RotationRange = FVector(360.0f, 360.0f, 360.0f);
    
    // Offset from surface
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    float SurfaceOffset = 1.0f;
    
    // Intersection bias - higher values concentrate spawns near intersections
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IntersectionBias = 0.7f;
    
    // Minimum distance from other spawned actors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    float MinSpawnDistance = 10.0f;
    
    // Enable auto-update when moving in editor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bAutoUpdateInEditor = true;
    
    // Manual spawn trigger
    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void SpawnActors();
    
    // Clear all spawned actors
    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void ClearSpawnedActors();
    
private:
    // Array to track spawned actors
    UPROPERTY()
    TArray<AActor*> SpawnedActors;
    
    // Helper function to detect geometry complexity
    float CalculateGeometryComplexity(const FVector& Location, const TArray<AActor*>& OverlappingActors);
    
    // Helper function to check if location is valid for spawning
    bool IsValidSpawnLocation(const FVector& Location);
};
