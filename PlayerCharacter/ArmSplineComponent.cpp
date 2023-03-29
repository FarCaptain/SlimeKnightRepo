// Fill out your copyright notice in the Description page of Project Settings.


#include "ArmSplineComponent.h"

#include "NiagaraFunctionLibrary.h"
#include "Enemies/GrabableEnemy.h"
#include "Engine/ICookInfo.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter/RCTCharacter.h"
#include "Components/SplineMeshComponent.h"

// Sets default values for this component's properties
UArmSplineComponent::UArmSplineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.03f;
	Spline = CreateDefaultSubobject<USplineComponent>("Spline");
	Spline->SetMobility(EComponentMobility::Movable);
	
	Spline->ClearSplinePoints();
}

void UArmSplineComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<ARCTCharacter>(GetOwner());
	if(ensure(PlayerCharacter))
	{
		for (int32 i = 0; i < SplinePointCount - 1; i++)
		{
			USplineMeshComponent* splineMesh = SplineMeshes[i];
			splineMesh->SetVisibility(true);
		}
	}
}

void UArmSplineComponent::SetUpSplineMeshes()
{
	for (int32 i = 0; i < SplinePointCount; i++)
	{
		Spline->AddSplinePointAtIndex(FVector(0, i * 5, 0), i, ESplineCoordinateSpace::World);
	}
	UE_LOG(LogTemp, Warning, TEXT("Hello: %d, %d"), SplinePointCount, Spline->GetNumberOfSplinePoints());
	
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

		FVector startLocation, startTangent, endLocation, endTangent;
		Spline->GetLocationAndTangentAtSplinePoint(i, startLocation, startTangent, ESplineCoordinateSpace::World);
		Spline->GetLocationAndTangentAtSplinePoint(i + 1, endLocation, endTangent, ESplineCoordinateSpace::World);
		splineMesh->SetStartAndEnd(startLocation, startTangent, endLocation, endTangent, true);
		splineMesh->SetMaterial(0, SplineMeshMaterial);
		
		splineMesh->SetCollisionProfileName(FName("SplineArm"));
		splineMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		splineMesh->OnComponentBeginOverlap.AddDynamic(this, &UArmSplineComponent::OnOverlapBegin);
		splineMesh->OnComponentEndOverlap.AddDynamic(this, &UArmSplineComponent::OnOverlapEnd);
	
		SplineMeshes.Add(splineMesh);
	}
}

void UArmSplineComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	SetUpSplineMeshes();
	Spline->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
}

void UArmSplineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	FVector shoulderPos = PlayerCharacter->GetShoulderJointLocation();
	
	FVector realHandPos = PlayerCharacter->GetRealHandJointLocation();
	FVector handTargetPos = PlayerCharacter->GetHandTargetLocation();

	LowerSamplePoint = FMath::Lerp(realHandPos, handTargetPos, LowerPointDeviation);
	UpperSamplePoint = FMath::Lerp(realHandPos, handTargetPos, UpperPointDeviation);

	LowerSamplePoint = FMath::Lerp(shoulderPos, LowerSamplePoint, LowerDistancePercentage);
	UpperSamplePoint = FMath::Lerp(shoulderPos, UpperSamplePoint, UpperDistancePercentage);
	
	float tValue = 0.0f;
	float tIncrement = 1.0f / ((float)SplinePointCount - 1.0f);
	for (int32 i = 0; i < SplinePointCount; i++)
	{
		FVector pos = CalculateCurvePoint(tValue, shoulderPos, LowerSamplePoint, UpperSamplePoint, realHandPos);
		Spline->SetLocationAtSplinePoint(i, pos, ESplineCoordinateSpace::World);
		tValue += tIncrement;
	}
	
	for (int32 i = 0; i < SplinePointCount - 1; i++)
	{
		USplineMeshComponent* splineMesh = SplineMeshes[i];

		FVector startLocation, startTangent, endLocation, endTangent;
		Spline->GetLocationAndTangentAtSplinePoint(i, startLocation, startTangent, ESplineCoordinateSpace::World);
		Spline->GetLocationAndTangentAtSplinePoint(i + 1, endLocation, endTangent, ESplineCoordinateSpace::World);
		splineMesh->SetStartAndEnd(startLocation, startTangent, endLocation, endTangent, true);
	}
}

//Cubic Bezier Curve
FVector UArmSplineComponent::CalculateCurvePoint(float tValue, FVector position0, FVector position1, FVector position2, FVector position3)
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
	
	if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass())
		&& IGrabableInterface::Execute_CanBeGrabbed(OtherActor))
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
	if (OtherActor->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass())
	&& OtherActor && IGrabableInterface::Execute_CanBeGrabbed(OtherActor))
	{
		// Arm Hit
		if(AEnemyBase* enemy = Cast<AEnemyBase>(OtherActor))
		{
			// might be bad for memory
			if(EnemyRecord.Contains(enemy))
			{
				-- EnemyRecord[enemy];
				enemy->StopPeriodicDamage();
			
				if(EnemyRecord[enemy] < 0)
					UE_LOG(LogTemp, Error, TEXT("[SplineArm] EnemyRecord became negative!"));
				if(EnemyRecord[enemy] == 0)
					EnemyRecord.Remove(enemy);
			}
		}
	}
}
