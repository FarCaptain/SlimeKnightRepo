// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RCTPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class RCT_API ARCTPlayerController : public APlayerController
{
	GENERATED_BODY()
public:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	void ArmX(float value);
	void ArmY(float value);
	void MoveX(float value);
	void MoveY(float value);
	void Grab();
	void LetGo();
	void Devour();

	bool bRestartOnDestroy = true;

private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class ARCTCharacter* myPawn;

	UFUNCTION()
	void PossessedPawnDestroyed(AActor* act);
};
