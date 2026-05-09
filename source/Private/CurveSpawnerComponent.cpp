// CurveSpawnerComponent.cpp
#include "CurveSpawnerComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UCurveSpawnerComponent::UCurveSpawnerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CurrentState = ECurveSpawnState::WaitingForFirstPoint;
}

void UCurveSpawnerComponent::BeginPlay()
{
    Super::BeginPlay();
    ResetCurve();
}

void UCurveSpawnerComponent::TryRegisterPoint()
{
    FVector HitLocation;
    if (!PerformAimTrace(HitLocation))
    {
        return;
    }

    if (CurrentState == ECurveSpawnState::WaitingForFirstPoint)
    {
        FirstPoint = HitLocation;
        CurrentState = ECurveSpawnState::WaitingForSecondPoint;

        if (bDrawDebug)
        {
            DrawDebugSphere(GetWorld(), FirstPoint, 15.f, 12, FColor::Green, false, 5.f);
        }
    }
    else if (CurrentState == ECurveSpawnState::WaitingForSecondPoint)
    {
        SecondPoint = HitLocation;

        if (bDrawDebug)
        {
            DrawDebugSphere(GetWorld(), SecondPoint, 15.f, 12, FColor::Red, false, 5.f);
        }

        SpawnCurveBetweenPoints();

        // Ready to place the next pair — previous curve stays in the world
        CurrentState = ECurveSpawnState::WaitingForFirstPoint;
    }
}


bool UCurveSpawnerComponent::PerformAimTrace(FVector& OutHitLocation) const
{
    AActor* Owner = GetOwner();
    if (!Owner) return false;

    // Prefer the camera's view (works with the default ThirdPerson FollowCamera)
    UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();
    FVector Start;
    FVector Forward;

    if (Camera)
    {
        Start = Camera->GetComponentLocation();
        Forward = Camera->GetForwardVector();
    }
    else
    {
        // Fallback: use actor eyes if no camera component exists
        FRotator EyeRot;
        Owner->GetActorEyesViewPoint(Start, EyeRot);
        Forward = EyeRot.Vector();
    }

    const FVector End = Start + (Forward * TraceDistance);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CurveSpawnerTrace), false, Owner);
    Params.AddIgnoredActor(Owner);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit, Start, End, ECC_Visibility, Params);

    if (bDrawDebug)
    {
        DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End,
            bHit ? FColor::Yellow : FColor::Red, false, 2.f, 0, 1.f);
    }

    if (bHit)
    {
        OutHitLocation = Hit.ImpactPoint;
        return true;
    }
    return false;
}

void UCurveSpawnerComponent::SpawnCurveBetweenPoints()
{
    UWorld* World = GetWorld();
    if (!World) return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.Owner = GetOwner();

    AActor* CurveActor = World->SpawnActor<AActor>(
        AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    if (!CurveActor) return;

    USceneComponent* Root = NewObject<USceneComponent>(CurveActor, TEXT("CurveRoot"));
    Root->SetMobility(EComponentMobility::Movable);
    CurveActor->SetRootComponent(Root);
    Root->RegisterComponent();

    if (!SplineMesh)
    {
        // Debug-only fallback: draw a curved line through a 3-point spline
        USplineComponent* DebugSpline = NewObject<USplineComponent>(CurveActor);
        DebugSpline->SetMobility(EComponentMobility::Movable);
        DebugSpline->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
        DebugSpline->RegisterComponent();
        DebugSpline->ClearSplinePoints();

        const FVector Mid = (FirstPoint + SecondPoint) * 0.5f
            + FVector(0.f, 0.f, CurveArcHeight);
        DebugSpline->AddSplinePoint(FirstPoint,  ESplineCoordinateSpace::World, false);
        DebugSpline->AddSplinePoint(Mid,         ESplineCoordinateSpace::World, false);
        DebugSpline->AddSplinePoint(SecondPoint, ESplineCoordinateSpace::World, true);

        if (bDrawDebug)
        {
            const int32 Segments = 30;
            const float Length = DebugSpline->GetSplineLength();
            FVector Prev = DebugSpline->GetLocationAtDistanceAlongSpline(
                0.f, ESplineCoordinateSpace::World);
            for (int32 i = 1; i <= Segments; ++i)
            {
                const float Dist = (Length * i) / Segments;
                const FVector Curr = DebugSpline->GetLocationAtDistanceAlongSpline(
                    Dist, ESplineCoordinateSpace::World);
                DrawDebugLine(World, Prev, Curr, FColor::Cyan, false, 10.f, 0, 3.f);
                Prev = Curr;
            }
        }

        SpawnedCurveActors.Add(CurveActor);
        return;
    }

    // Compute tangents so a single SplineMeshComponent arcs through the midpoint.
    // The tangent magnitude controls how "pulled" the curve is toward the midpoint.
    // A good starting point is the chord length, scaled by how high we want the arc.
    const FVector AB     = SecondPoint - FirstPoint;
    const float ChordLen = AB.Size();
    const FVector Dir    = ChordLen > KINDA_SMALL_NUMBER ? AB / ChordLen : FVector::ForwardVector;

    // Tangent at start: forward along the chord, plus an upward bias to create the arc.
    // Tangent at end:   forward along the chord, minus an upward bias (so it curves down toward B).
    const FVector UpBias       = FVector::UpVector * (CurveArcHeight * 2.f);
    const FVector StartTangent = (Dir * ChordLen) + UpBias;
    const FVector EndTangent   = (Dir * ChordLen) - UpBias;

    USplineMeshComponent* SMC = NewObject<USplineMeshComponent>(CurveActor);
    SMC->SetMobility(EComponentMobility::Movable);
    SMC->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
    SMC->RegisterComponent();

    SMC->SetStaticMesh(SplineMesh);
    if (SplineMaterial)
    {
        SMC->SetMaterial(0, SplineMaterial);
    }
    SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // SetStartAndEnd takes positions/tangents in the component's local space.
    // The actor is at world origin, so world == local here.
    SMC->SetStartAndEnd(FirstPoint, StartTangent, SecondPoint, EndTangent, true);

    SpawnedCurveActors.Add(CurveActor);
}

void UCurveSpawnerComponent::ResetCurve()
{
    // Just clears the in-progress state; placed curves are untouched
    CurrentState = ECurveSpawnState::WaitingForFirstPoint;
}

void UCurveSpawnerComponent::ClearAllCurves()
{
    for (AActor* Actor : SpawnedCurveActors)
    {
        if (Actor) Actor->Destroy();
    }
    SpawnedCurveActors.Empty();
    CurrentState = ECurveSpawnState::WaitingForFirstPoint;
}