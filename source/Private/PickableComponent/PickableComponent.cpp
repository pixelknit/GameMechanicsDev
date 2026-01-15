// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/PickableComponent.h"

#include "Components/ShapeComponent.h"

// Sets default values for this component's properties
UPickableComponent::UPickableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UPickableComponent::BeginPlay()
{
	Super::BeginPlay();
	if(bAutoRegisterToCollision)
	{
		TArray<UShapeComponent*> Shapes;
		GetOwner()->GetComponents<UShapeComponent>(Shapes);
		for(UShapeComponent* Shape : Shapes)
		{
			Shape->OnComponentBeginOverlap.AddDynamic(this, &UPickableComponent::OnBeginOverlap);
		}
	}
	// ...
	
}

void UPickableComponent::PickUp(AActor* Instigator)
{
	OnPicked.Broadcast(Instigator);
	if(bDestroyActorOnPicked)
	{
		GetOwner()->Destroy();
	}
}

// Called every frame
void UPickableComponent::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	PickUp(OtherActor);	
}
