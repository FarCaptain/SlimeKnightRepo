// 2022 - 2023 Lucas Qu @SlimeKnight

#include "ArmSplineComponent.h"

#include "DynamicCameraShake.h"
#include "NiagaraFunctionLibrary.h"
#include "Enemies/GrabableEnemy.h"
#include "Engine/ICookInfo.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter/RCTCharacter.h"
#include "Components/SplineMeshComponent.h"
#include "Components/TimelineComponent.h"

UArmSplineComponent::UArmSplineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	//PrimaryComponentTick.TickInterval = 0.03f;
	Spline = CreateDefaultSubobject<USplineComponent>("Spline");
	Spline->SetMobility(EComponentMobility::Movable);

	Spline->ClearSplinePoints();
	HandInput = FVector2D::Zero();
}

void UArmSplineComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	SetUpSplineMeshes();
	Spline->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);

	SetUpBulgeParameters();
}

void UArmSplineComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<ARCTCharacter>(GetOwner());
	ensure(PlayerCharacter);
	
	// Setup the bulging timeline
	FOnTimelineFloat ProgressUpdate;
	ProgressUpdate.BindUFunction(this, FName("DevourBulgeUpdate"));

	FOnTimelineEvent FinishedEvent;
	FinishedEvent.BindUFunction(this, FName("DevourBulgeFinished"));

	DevourBulgeTimeline.AddInterpFloat(DevourBulgeMovingCurve, ProgressUpdate);
	DevourBulgeTimeline.SetTimelineFinishedFunc(FinishedEvent);

	DevourBulgeTimeline.SetLooping(true);
}

void UArmSplineComponent::StartDevourBulgeTimeline()
{
	if(bIsBulging)
		return;

	bIsBulging = true;
	BulgeCenterIdx = SplinePointCount - 1 - BulgeRadius;
	DevourBulgeTimeline.PlayFromStart();
}

void UArmSplineComponent::StopDevourBulgeTimeline()
{
	if(bIsBulging)
	{
		BulgeCenterIdx = SplinePointCount - 1 - BulgeRadius;
		bIsBulging = false;
		DevourBulgeTimeline.Stop();
	}
}


void UArmSplineComponent::DevourBulgeUpdate(float Alpha)
{
	float splineMeshCount = static_cast<float>(SplinePointCount) - 1.0f;

	float progress = FMath::Lerp(splineMeshCount - BulgeRadius, BulgeRadius - BulgeTimelineOffset, Alpha);
	BulgeCenterIdx = FMath::RoundToInt(progress);
}

void UArmSplineComponent::DevourBulgeFinished()
{
	BulgeCenterIdx = SplinePointCount - 1 - BulgeRadius;
}

void UArmSplineComponent::SetUpBulgeParameters()
{
	int32 bulgeSize = BulgeRadius * 2 + 1;

	//sin wave
	BulgeScaleIncrements.SetNum(bulgeSize + 1);
	for (int32 i = 0; i < bulgeSize + 1; i++)
	{
		BulgeScaleIncrements[i] = BulgeAmplitude * sin(PI / bulgeSize * i);
	}
}

void UArmSplineComponent::SetUpSplineMeshes()
{
	for (int32 i = 0; i < SplinePointCount; i++)
	{
		Spline->AddSplinePointAtIndex(FVector(0, i * 5, 0), i, ESplineCoordinateSpace::World, false);
	}
	Spline->UpdateSpline();

	if (SplineMeshes.Num() > 0)
		SplineMeshes.Empty();

	for (int32 i = 0; i < SplinePointCount - 1; i++)
	{
		USplineMeshComponent* splineMesh = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass());

		splineMesh->SetStaticMesh(SplineMeshStaticMesh);
		splineMesh->SetMobility(EComponentMobility::Movable);
		splineMesh->SetForwardAxis(ESplineMeshAxis::Z);

		splineMesh->SetStartScale(SplineMeshStartScale);
		splineMesh->SetEndScale(SplineMeshEndScale);

		splineMesh->RegisterComponentWithWorld(GetWorld());
		// splineMesh->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
		
		splineMesh->SetMaterial(0, SplineMeshMaterial);

		splineMesh->SetCollisionProfileName(FName("SplineArm"));
		splineMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		splineMesh->OnComponentBeginOverlap.AddDynamic(this, &UArmSplineComponent::OnOverlapBegin);
		splineMesh->OnComponentEndOverlap.AddDynamic(this, &UArmSplineComponent::OnOverlapEnd);

		SplineMeshes.Add(splineMesh);
	}
}

void UArmSplineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DevourBulgeTimeline.TickTimeline(DeltaTime * DevourBulgeMovingSpeed);
	
	FVector shoulderPos = PlayerCharacter->GetShoulderJointLocation();
	
	FVector realHandPos = PlayerCharacter->GetRealHandJointLocation();
	FVector handTargetPos = PlayerCharacter->GetHandTargetLocation();

	LowerSamplePoint = FMath::Lerp(realHandPos, handTargetPos, LowerPointDeviation);
	UpperSamplePoint = FMath::Lerp(realHandPos, handTargetPos, UpperPointDeviation);

	LowerSamplePoint = FMath::Lerp(shoulderPos, LowerSamplePoint, LowerDistancePercentage);
	UpperSamplePoint = FMath::Lerp(shoulderPos, UpperSamplePoint, UpperDistancePercentage);
	
	float tValue = 0.0f;
	float tIncrement = 1.0f / (static_cast<float> (SplinePointCount) - 1.0f);
	for (int32 i = 0; i < SplinePointCount; i++)
	{
		FVector pos = CalculateCurvePoint(tValue, shoulderPos, LowerSamplePoint, UpperSamplePoint, realHandPos);
		Spline->SetLocationAtSplinePoint(i, pos, ESplineCoordinateSpace::World, false);
		tValue += tIncrement;
	}
	Spline->UpdateSpline();
	
	for (int32 i = 0; i < SplinePointCount - 1; i++)
	{
		USplineMeshComponent* splineMesh = SplineMeshes[i];
		FVector startLocation, startTangent, endLocation, endTangent;
		Spline->GetLocationAndTangentAtSplinePoint(i, startLocation, startTangent, ESplineCoordinateSpace::World);
		Spline->GetLocationAndTangentAtSplinePoint(i + 1, endLocation, endTangent, ESplineCoordinateSpace::World);
		// splineMesh->SetStartAndEnd(startLocation, startTangent, endLocation, endTangent, true);
		splineMesh->SetStartAndEnd(startLocation, startTangent * TangentScale, endLocation, endTangent * TangentScale, false);

		FVector2D startScale = SplineMeshStartScale;
		FVector2D endScale = SplineMeshEndScale;
		if(bIsBulging)
		{
			int32 startIdx = BulgeCenterIdx - BulgeRadius;
			int32 endIdx = BulgeCenterIdx + BulgeRadius;
			bool bIsBulgeInRange = startIdx >= 0 && endIdx < SplineMeshes.Num();
			bool bIsIndexInBulge = i >= startIdx && i <= endIdx;

			if( bIsBulgeInRange && bIsIndexInBulge)
			{
				startScale =  SplineMeshStartScale + BulgeScaleIncrements[i - startIdx];
				endScale = SplineMeshEndScale + BulgeScaleIncrements[i - startIdx + 1];
			}
		}
		SetSplineMeshScale(splineMesh, startScale, endScale);
		splineMesh->UpdateMesh();

		// make it sweep
		// FHitResult HitResult;
		// splineMesh->MoveComponent(FVector::ZeroVector, splineMesh->GetComponentRotation(), true, &HitResult);
	}
}

//Cubic Bezier Curve
FVector UArmSplineComponent::CalculateCurvePoint(float tValue, FVector& position0, FVector& position1, FVector& position2, FVector& position3)
{
	float oneMinusT = 1.0f - tValue;
	float oneMinusTSquare = oneMinusT * oneMinusT;
	float oneMinusTCube = oneMinusTSquare * oneMinusT;

	float tSquare = tValue * tValue;
	float tCube = tSquare * tValue;

	FVector term1 = oneMinusTCube * position0;
	FVector term2 = 3.0f * oneMinusTSquare * tValue * position1;
	FVector term3 = 3.0f * oneMinusT * tSquare * position2;
	FVector term4 = tCube * position3;

	return term1 + term2 + term3 + term4;
}

UNiagaraSystem* UArmSplineComponent::GetArmHitParticleEffect() const
{
	return ArmHitParticleEffect;
}


void UArmSplineComponent::OnEnemyOverlaped_Implementation(AEnemyBase* enemy)
{
	enemy->TakeDamage(ArmHitDamage, FPointDamageEvent(), PlayerCharacter->GetInstigatorController(), PlayerCharacter);
	enemy->Stun();
	UNiagaraFunctionLibrary::SpawnSystemAtLocation( GetWorld(), ArmHitParticleEffect, enemy->GetTransform().GetLocation());
	UGameplayStatics::PlaySound2D(GetWorld(), ArmHitSound);
	armHitEvent.Broadcast(enemy);
	
	if(armHitCamShake)
	{
		if(HitPredict != FVector2D::Zero())
		{
			UDynamicCameraShake::UpdateCamShake(armHitCamShake, HitPredict);
			UGameplayStatics::PlayWorldCameraShake(this, armHitCamShake, enemy->GetActorLocation(), 0, 50000, 1, true);
		}
		else
		{
			// fall back cam shake
		}
	}
}

void UArmSplineComponent::SetSplineMeshMaterial(UMaterialInstance* mat)
{
	SplineMeshMaterial = mat;
	for (int32 i = 0; i < SplinePointCount - 1; i++)
	{
		USplineMeshComponent* splineMesh = SplineMeshes[i];
		splineMesh->SetMaterial(0, SplineMeshMaterial);
		splineMesh->UpdateMesh();
	}
}

void UArmSplineComponent::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor && OtherActor == GetOwner())
	{
		return;
	}

	// DrawDebugString(GetWorld(), OtherActor->GetActorLocation(), AActor::GetDebugName(OtherActor), nullptr, FColor::Yellow, 6.0f, true);
	// DrawDebugString(GetWorld(), OtherActor->GetActorLocation(), OtherComp->GetName(), nullptr, FColor::Yellow, 6.0f, true);
	
	if (OtherActor->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass()) && !IGrabableInterface::Execute_IsGrabbed(OtherActor))
	{
		// Arm Hit
		if(AEnemyBase* enemy = Cast<AEnemyBase>(OtherActor))
		{
			if(!EnemyRecord.Contains(enemy))
			{
				EnemyRecord.Add(enemy, 1);

				
				OnEnemyOverlaped(enemy);
				enemy->TakePeriodicDamage(ArmStayDamage, ArmStayDamageTimeInterval);
			}
			else
			{
				EnemyRecord[enemy] ++;
			}
		}
	}
}

void UArmSplineComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass()) && !IGrabableInterface::Execute_IsGrabbed(OtherActor))
	{
		// Arm Hit
		if(AEnemyBase* enemy = Cast<AEnemyBase>(OtherActor))
		{
			if(EnemyRecord.Contains(enemy))
			{
				-- EnemyRecord[enemy];
				enemy->StopPeriodicDamage();
			
				if(EnemyRecord[enemy] < 0)
				{
					UE_LOG(LogTemp, Error, TEXT("[SplineArm] EnemyRecord became negative!"));
				}
				if(EnemyRecord[enemy] == 0)
				{
					EnemyRecord.Remove(enemy);
				}
			}
		}
	}
}

void UArmSplineComponent::RemoveOverlapRecord(AEnemyBase* enemy)
{
	if(EnemyRecord.Contains(enemy))
	{
		EnemyRecord.Remove(enemy);
	}
}

void UArmSplineComponent::UpdateHandInput(FVector2D inputVector)
{
	FVector2D currentInput = inputVector.GetSafeNormal();
	HitPredict = FVector2D::Zero();
	if(HandInput != FVector2D::Zero())
	{
		float cross = FVector2D::CrossProduct(HandInput, currentInput);
		if(cross > 0)
		{
			HitPredict = FVector2D(currentInput.Y, -currentInput.X);
		}
		else if(cross < 0)
		{
			HitPredict = FVector2D(currentInput.Y, -currentInput.X);
		}
	}
	HandInput = currentInput;
}
