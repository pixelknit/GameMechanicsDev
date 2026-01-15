// DecalInstanceProjector.cpp
#include "ActorUtils/DecalInstanceProjector.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"

ADecalInstanceProjector::ADecalInstanceProjector()
{
    PrimaryActorTick.bCanEverTick = false; // No tick needed!
    
    // Create root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    
    // Create box component
    ProjectorBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ProjectorBox"));
    ProjectorBox->SetupAttachment(RootComponent);
    ProjectorBox->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
    ProjectorBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProjectorBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    ProjectorBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
    ProjectorBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    ProjectorBox->SetGenerateOverlapEvents(true);
    
    // Editor-friendly settings
    SetActorEnableCollision(true);
}

void ADecalInstanceProjector::BeginPlay()
{
    Super::BeginPlay();
}

#if WITH_EDITOR
void ADecalInstanceProjector::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    // Auto-update when properties change in editor
    if (bAutoUpdateInEditor && PropertyChangedEvent.Property != nullptr)
    {
        FName PropertyName = PropertyChangedEvent.Property->GetFName();
        
        // Update on relevant property changes
        if (PropertyName == GET_MEMBER_NAME_CHECKED(ADecalInstanceProjector, ActorToSpawn) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(ADecalInstanceProjector, SpawnDensity) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(ADecalInstanceProjector, ScaleRange) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(ADecalInstanceProjector, RotationRange) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(ADecalInstanceProjector, SurfaceOffset) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(ADecalInstanceProjector, IntersectionBias) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(ADecalInstanceProjector, MinSpawnDistance))
        {
            ClearSpawnedActors();
            SpawnActors();
        }
    }
}

void ADecalInstanceProjector::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);
    
    // Only update when move is finished to avoid spamming during drag
    if (bFinished && bAutoUpdateInEditor)
    {
        ClearSpawnedActors();
        SpawnActors();
    }
}
#endif

void ADecalInstanceProjector::SpawnActors()
{
    if (!ActorToSpawn && !StaticMeshToSpawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("DecalInstanceProjector: No ActorToSpawn or StaticMeshToSpawn set!"));
        return;
    }
    
    // Clear existing spawns
    ClearSpawnedActors();
    
    // Query for actors within the box using overlap test
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape BoxShape = FCollisionShape::MakeBox(ProjectorBox->GetScaledBoxExtent());
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    
    GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        ProjectorBox->GetComponentLocation(),
        ProjectorBox->GetComponentQuat(),
        ECC_WorldStatic,
        BoxShape,
        QueryParams
    );
    
    // Extract unique actors from overlap results
    TArray<AActor*> OverlappingActors;
    for (const FOverlapResult& Result : OverlapResults)
    {
        if (Result.GetActor() && !OverlappingActors.Contains(Result.GetActor()))
        {
            OverlappingActors.Add(Result.GetActor());
        }
    }
    
    if (OverlappingActors.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("DecalInstanceProjector: No overlapping actors found! Make sure your cubes have collision enabled."));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("DecalInstanceProjector: Found %d overlapping actors"), OverlappingActors.Num());
    
    FVector BoxExtent = ProjectorBox->GetScaledBoxExtent();
    FVector BoxCenter = ProjectorBox->GetComponentLocation();
    FRotator BoxRotation = ProjectorBox->GetComponentRotation();
    
    // Generate spawn positions
    for (int32 i = 0; i < SpawnDensity; i++)
    {
        // Random position within box (local space)
        FVector LocalPos = FVector(
            FMath::RandRange(-BoxExtent.X, BoxExtent.X),
            FMath::RandRange(-BoxExtent.Y, BoxExtent.Y),
            FMath::RandRange(-BoxExtent.Z, BoxExtent.Z)
        );
        
        // Transform to world space
        FVector WorldPos = BoxCenter + BoxRotation.RotateVector(LocalPos);
        
        // Calculate geometry complexity at this location
        float Complexity = CalculateGeometryComplexity(WorldPos, OverlappingActors);
        
        // Use intersection bias to determine if we should spawn here
        float SpawnChance = FMath::Lerp(1.0f, Complexity, IntersectionBias);
        if (FMath::FRand() > SpawnChance)
        {
            continue;
        }
        
        // Raycast down to find surface
        FHitResult Hit;
        FVector StartTrace = WorldPos + FVector(0, 0, BoxExtent.Z);
        FVector EndTrace = WorldPos - FVector(0, 0, BoxExtent.Z * 2);
        
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        for (AActor* SpawnedActor : SpawnedActors)
        {
            QueryParams.AddIgnoredActor(SpawnedActor);
        }
        
        if (GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_Visibility, QueryParams))
        {
            // Check if this location is valid (not too close to other spawns)
            FVector SpawnLocation = Hit.Location + Hit.Normal * SurfaceOffset;
            
            if (!IsValidSpawnLocation(SpawnLocation))
            {
                continue;
            }
            
            // Random rotation
            FRotator SpawnRotation = FRotator(
                FMath::RandRange(-RotationRange.X, RotationRange.X),
                FMath::RandRange(-RotationRange.Y, RotationRange.Y),
                FMath::RandRange(-RotationRange.Z, RotationRange.Z)
            );
            
            // Align to surface normal (optional - can be toggled)
            FRotator SurfaceRotation = Hit.Normal.Rotation();
            SpawnRotation = FRotator(SurfaceRotation.Pitch, SpawnRotation.Yaw, SpawnRotation.Roll);
            
            // Spawn actor
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = this;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            
            AActor* SpawnedActor = nullptr;
            
            // Option 1: Spawn the specified actor class
            if (ActorToSpawn)
            {
                SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
            }
            // Option 2: Spawn a simple static mesh actor
            else if (StaticMeshToSpawn)
            {
                SpawnedActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
                
                if (SpawnedActor)
                {
                    UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(SpawnedActor, UStaticMeshComponent::StaticClass(), TEXT("SpawnedMesh"));
                    MeshComp->SetStaticMesh(StaticMeshToSpawn);
                    
                    if (SpawnedMeshMaterial)
                    {
                        MeshComp->SetMaterial(0, SpawnedMeshMaterial);
                    }
                    
                    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    MeshComp->RegisterComponent();
                    SpawnedActor->SetRootComponent(MeshComp);
                }
            }
            
            if (SpawnedActor)
            {
                // Random scale
                float RandomScale = FMath::RandRange(ScaleRange.X, ScaleRange.Y);
                SpawnedActor->SetActorScale3D(FVector(RandomScale));
                
                SpawnedActors.Add(SpawnedActor);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("DecalInstanceProjector: Spawned %d actors"), SpawnedActors.Num());
}

void ADecalInstanceProjector::ClearSpawnedActors()
{
    for (AActor* SpawnedActor : SpawnedActors)
    {
        if (SpawnedActor && IsValid(SpawnedActor))
        {
            SpawnedActor->Destroy();
        }
    }
    SpawnedActors.Empty();
}

float ADecalInstanceProjector::CalculateGeometryComplexity(const FVector& Location, const TArray<AActor*>& OverlappingActors)
{
    // Cast rays in multiple directions to detect geometry density
    const int32 NumRays = 8;
    int32 HitCount = 0;
    float TotalDistance = 0.0f;
    float TraceDistance = 100.0f;
    
    for (int32 i = 0; i < NumRays; i++)
    {
        float Angle = (360.0f / NumRays) * i;
        FVector Direction = FRotator(0, Angle, 0).Vector();
        
        FHitResult Hit;
        FVector Start = Location;
        FVector End = Location + Direction * TraceDistance;
        
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
        {
            HitCount++;
            TotalDistance += Hit.Distance;
        }
    }
    
    // More hits and shorter distances = higher complexity
    if (HitCount == 0) return 0.0f;
    
    float AverageDistance = TotalDistance / HitCount;
    float Complexity = (float)HitCount / NumRays;
    float DistanceFactor = 1.0f - FMath::Clamp(AverageDistance / TraceDistance, 0.0f, 1.0f);
    
    return (Complexity + DistanceFactor) * 0.5f;
}

bool ADecalInstanceProjector::IsValidSpawnLocation(const FVector& Location)
{
    for (const AActor* SpawnedActor : SpawnedActors)
    {
        if (SpawnedActor && IsValid(SpawnedActor))
        {
            float Distance = FVector::Distance(Location, SpawnedActor->GetActorLocation());
            if (Distance < MinSpawnDistance)
            {
                return false;
            }
        }
    }
    return true;
}