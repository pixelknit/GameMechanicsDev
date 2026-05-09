// CurveSpawnerComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CurveSpawnerComponent.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ECurveSpawnState : uint8
{
    WaitingForFirstPoint    UMETA(DisplayName = "Waiting For First Point"),
    WaitingForSecondPoint   UMETA(DisplayName = "Waiting For Second Point"),
    CurveSpawned            UMETA(DisplayName = "Curve Spawned")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LEVELMECHANICSDEV_API UCurveSpawnerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCurveSpawnerComponent();

    // Call this from your Character (e.g. on key press) to register a hit point
    UFUNCTION(BlueprintCallable, Category = "Curve Spawner")
    void TryRegisterPoint();

    // Reset state so the player can place a new pair of points
    UFUNCTION(BlueprintCallable, Category = "Curve Spawner")
    void ResetCurve();

    UPROPERTY(BlueprintReadOnly, Category = "Curve Spawner")
    ECurveSpawnState CurrentState;

    // Add a public function to clear all curves on demand
    UFUNCTION(BlueprintCallable, Category = "Curve Spawner")
    void ClearAllCurves();

protected:
    virtual void BeginPlay() override;



    // How far the trace reaches from the camera
    UPROPERTY(EditAnywhere, Category = "Curve Spawner|Trace")
    float TraceDistance = 10000.f;

    // How high the curve arches between the two points (in cm)
    UPROPERTY(EditAnywhere, Category = "Curve Spawner|Curve")
    float CurveArcHeight = 200.f;

    // Mesh used as the visual segment of the spline
    UPROPERTY(EditAnywhere, Category = "Curve Spawner|Curve")
    TObjectPtr<UStaticMesh> SplineMesh;

    UPROPERTY(EditAnywhere, Category = "Curve Spawner|Curve")
    TObjectPtr<UMaterialInterface> SplineMaterial;

    // Whether to draw debug lines/spheres for the trace and points
    UPROPERTY(EditAnywhere, Category = "Curve Spawner|Debug")
    bool bDrawDebug = true;

private:
    bool PerformAimTrace(FVector& OutHitLocation) const;
    void SpawnCurveBetweenPoints();

    FVector FirstPoint;
    FVector SecondPoint;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> SpawnedCurveActors;

};