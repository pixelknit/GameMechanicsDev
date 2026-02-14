// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DimensionalVisibilityComponent.generated.h"

UENUM(BlueprintType)
enum class EDimensionType : uint8
{
	DimensionA UMETA(DisplayName = "Dimension A"),
	DimensionB UMETA(DisplayName = "Dimension B")
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DR_TEST_API UDimensionalVisibilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDimensionalVisibilityComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dimension")
	EDimensionType BelongsToDimension;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dimension")
	bool bStartsVisible;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dimension")
	bool bAffectsCollision;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable, Category = "Dimension")
	void UpdateVisibility(EDimensionType CurrentDimension);

	UFUNCTION(BlueprintCallable, Category = "Dimension")
	void ToggleDimension();
		
};
