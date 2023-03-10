// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "ArmSplineComponent.generated.h"

class AEnemyBase;
UENUM(BlueprintType)
enum class EArmCollision : uint8 {
	VE_NoCollision       UMETA(DisplayName = "NoCollision"),
	VE_QueryOnly        UMETA(DisplayName = "QueryOnly"),
	VE_PhysicsOnly        UMETA(DisplayName = "PhysicsOnly"),
	VE_QueryAndPhysics        UMETA(DisplayName = "QueryAndPhysics"),
};

UCLASS( Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RCT_API UArmSplineComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPROPERTY(VisibleAnywhere, Category = "Spline")
		USplineComponent* Spline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
		UStaticMesh* SplineMeshStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline")
		UMaterialInstance* SplineMeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
		int32 SplinePointCount = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
		FVector2D SplineMeshStartScale = FVector2D(0.6, 0.6);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
		FVector2D SplineMeshEndScale = FVector2D(0.65, 0.65);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
		float HideArmThreshold = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
		EArmCollision ArmCollisionType = EArmCollision::VE_QueryOnly;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="ArmHit")
		float ArmHitDamage = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ArmHit")
		UNiagaraSystem* ArmHitParticleEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ArmHit")
		USoundBase* ArmHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		AActor* PlayerActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		USkeletalMeshComponent* HandTargetComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		USkeletalMeshComponent* RealHandComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		float LowerPointDeviation = 0.9f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		float UpperPointDeviation = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		float LowerDistancePercentage = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		float UpperDistancePercentage = 0.75;

	// Sets default values for this component's properties
	UArmSplineComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Spline")
	TArray<USplineMeshComponent*> SplineMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Positions")
	FVector LowerSamplePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Positions")
	FVector UpperSamplePoint;

	// Only call stuff once with the whole arm instead of doing it for each spline mesh
	TMap<AEnemyBase*, int> EnemyRecord;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION( )
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION( )
	void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UNiagaraSystem* GetArmHitParticleEffect() const;
	
	UFUNCTION(BlueprintNativeEvent)
	void OnEnemyOverlaped(AEnemyBase* enemy);

	UFUNCTION(BlueprintCallable)
	void SetSplineMeshMaterial(UMaterialInstance* mat);
	
private:
	FVector CalculateCurvePoint(float tValue, FVector position0, FVector position1, FVector position2, FVector position3);
};
