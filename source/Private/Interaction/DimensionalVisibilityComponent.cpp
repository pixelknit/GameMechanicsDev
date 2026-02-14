#include "Interaction/DimensionalVisibilityComponent.h"

UDimensionalVisibilityComponent::UDimensionalVisibilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BelongsToDimension = EDimensionType::DimensionA;
	bStartsVisible = true;
	bAffectsCollision = true;
}

void UDimensionalVisibilityComponent::BeginPlay()
{
	Super::BeginPlay();
    
	// Set initial state
	AActor* Owner = GetOwner();
	if (Owner)
	{
		const bool bShouldBeVisible = (BelongsToDimension == EDimensionType::DimensionA) ? bStartsVisible : !bStartsVisible;
        
		Owner->SetActorHiddenInGame(!bShouldBeVisible);
        
		if (bAffectsCollision)
		{
			Owner->SetActorEnableCollision(bShouldBeVisible);
		}
	}
}

void UDimensionalVisibilityComponent::UpdateVisibility(EDimensionType CurrentDimension)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const bool bShouldBeVisible = (BelongsToDimension == CurrentDimension);
    
	Owner->SetActorHiddenInGame(!bShouldBeVisible);
    
	if (bAffectsCollision)
	{
		Owner->SetActorEnableCollision(bShouldBeVisible);
	}
}

void UDimensionalVisibilityComponent::ToggleDimension()
{
	// Helper to flip between dimensions
	BelongsToDimension = (BelongsToDimension == EDimensionType::DimensionA) 
		? EDimensionType::DimensionB 
		: EDimensionType::DimensionA;
        
	UpdateVisibility(EDimensionType::DimensionA); // Reset to check current
}