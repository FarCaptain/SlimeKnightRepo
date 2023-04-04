// 2022 - 2023 Lucas Qu @SlimeKnight

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "ArmSplineComponent.generated.h"

class AEnemyBase;
class ARCTCharacter;
class USplineMeshComponent;

UENUM(BlueprintType)
enum class EArmCollision : uint8 {
	VE_NoCollision       UMETA(DisplayName = "NoCollision"),
	VE_QueryOnly        UMETA(DisplayName = "QueryOnly"),
	VE_PhysicsOnly        UMETA(DisplayName = "PhysicsOnly"),
	VE_QueryAndPhysics        UMETA(DisplayName = "QueryAndPhysics"),
};

UCLASS( Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RCT_API UArmSplineComponent : public USceneComponent
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
		FVector TangentScale = FVector(1);


	UPROPERTY(BlueprintReadWrite, Category="ArmHit")
		float ArmHitDamage = 1.f;
	UPROPERTY(BlueprintReadWrite, Category="ArmHit")
		float ArmStayDamage = 0.2f;
	UPROPERTY(BlueprintReadWrite, Category="ArmHit")
		float ArmStayDamageTimeInterval = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ArmHit")
		UNiagaraSystem* ArmHitParticleEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ArmHit")
		USoundBase* ArmHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		ARCTCharacter* PlayerCharacter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		float LowerPointDeviation = 0.9f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		float UpperPointDeviation = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		float LowerDistancePercentage = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positions")
		float UpperDistancePercentage = 0.75;

	UFUNCTION()
	void StartDevourBulgeTimeline();

	UFUNCTION()
	void StopDevourBulgeTimeline();

	UArmSplineComponent();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	TArray<USplineMeshComponent*> SplineMeshes;

	UPROPERTY(EditDefaultsOnly, Category = "DevourEffect")
	UCurveFloat* DevourBulgeMovingCurve;

	UPROPERTY(EditDefaultsOnly, Category = "DevourEffect")
	float DevourBulgeMovingSpeed = 0.5f;

	// Used to offset min value so we can get a pause in between loops
	UPROPERTY(EditDefaultsOnly, Category = "DevourEffect")
	float BulgeTimelineOffset = 0;

	UPROPERTY(EditDefaultsOnly, Category = "DevourEffect")
	float BulgeAmplitude = 2.3f;

	UPROPERTY(VisibleAnywhere, Category = "DevourEffect")
	TArray<float> BulgeScaleIncrements;
	
	void SetUpBulgeParameters();

	UPROPERTY()
	FTimeline DevourBulgeTimeline;
	
	int32 BulgeCenterIdx;
	
	int32 BulgeRadius = 4;
	
	bool bIsBulging = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Positions")
	FVector LowerSamplePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Positions")
	FVector UpperSamplePoint;

	// Only call stuff once with the whole arm instead of doing it for each spline mesh
	TMap<AEnemyBase*, int> EnemyRecord;

	virtual void BeginPlay() override;

	UFUNCTION()
	void DevourBulgeUpdate(float Alpha);
	UFUNCTION()
	void DevourBulgeFinished();

	void SetUpSplineMeshes();

	virtual void OnComponentCreated() override;

	void SetSplineMeshScale(USplineMeshComponent* splineMesh, const FVector2D& startScale, const FVector2D& endScale)
	{
		const FVector2D& originalStartScale = splineMesh->GetStartScale();
		const FVector2D& originalEndScale = splineMesh->GetEndScale();

		if(originalStartScale != startScale)
		{
			splineMesh->SetStartScale(startScale, false);
		}
		if(originalEndScale != endScale)
		{
			splineMesh->SetEndScale(endScale, false);
		}
	}

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
	FVector CalculateCurvePoint(float tValue, FVector& position0, FVector& position1, FVector& position2, FVector& position3);
};
