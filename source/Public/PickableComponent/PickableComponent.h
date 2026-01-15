// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PickableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPicked, AActor*, Instigator);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DR_TEST_API UPickableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPickableComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	bool bAutoRegisterToCollision = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	bool bDestroyActorOnPicked = false;
	
	UPROPERTY(BlueprintAssignable)
	FOnPicked OnPicked;

	UFUNCTION(BlueprintCallable, Category="Interaction")
	void PickUp(AActor* Instigator);

protected:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
		
};
