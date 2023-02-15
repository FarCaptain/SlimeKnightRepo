// Fill out your copyright notice in the Description page of Project Settings.


#include "ArmSplineComponent.h"

#include "NiagaraFunctionLibrary.h"
#include "Enemies/GrabableEnemy.h"
#include "Engine/ICookInfo.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UArmSplineComponent::UArmSplineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.03f;
	Spline = CreateDefaultSubobject<USplineComponent>("Spline");
	//SplineMeshStaticMesh = CreateDefaultSubobject<UStaticMesh>(TEXT("MyFMesh"))

	Spline->ClearSplinePoints();

	if (SplineMeshes.Num() > 0)
		SplineMeshes.Empty();

	for (int32 i = 0; i < SplinePointCount; i++)
	{
		Spline->AddSplinePointAtIndex(FVector(0, i * 5, 0), i, ESplineCoordinateSpace::World);
	}

	UE_LOG(LogTemp, Warning, TEXT("Hello: %d, %d"), SplinePointCount, Spline->GetNumberOfSplinePoints());
}


// Called when the game starts
void UArmSplineComponent::BeginPlay()
{
	Super::BeginPlay();

	// doing intialization in Beginplay because 
	PlayerActor = GetOwner();

	ECollisionEnabled::Type collisionType = ECollisionEnabled::NoCollision;
	switch (ArmCollisionType)
	{
	case EArmCollision::VE_NoCollision:
		collisionType = ECollisionEnabled::NoCollision;
		break;
	case EArmCollision::VE_QueryOnly:
		collisionType = ECollisionEnabled::QueryOnly;
		break;
	case EArmCollision::VE_PhysicsOnly:
		collisionType = ECollisionEnabled::PhysicsOnly;
		break;
	case EArmCollision::VE_QueryAndPhysics:
		collisionType = ECollisionEnabled::QueryAndPhysics;
		break;
	}

	for (int32 i = 0; i < SplinePointCount - 1; i++)
	{
		USplineMeshComponent* splineMesh = NewObject<USplineMeshComponent>(PlayerActor, USplineMeshComponent::StaticClass());

		splineMesh->SetStaticMesh(SplineMeshStaticMesh);
		splineMesh->SetMobility(EComponentMobility::Movable);
		splineMesh->SetForwardAxis(ESplineMeshAxis::Z);

		splineMesh->SetStartScale(SplineMeshStartScale);
		splineMesh->SetEndScale(SplineMeshEndScale);

		//splineMesh->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		splineMesh->RegisterComponentWithWorld(GetWorld());
		splineMesh->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);

		FVector startLocation, startTangent, endLocation, endTangent;
		Spline->GetLocationAndTangentAtSplinePoint(i, startLocation, startTangent, ESplineCoordinateSpace::Local);
		Spline->GetLocationAndTangentAtSplinePoint(i + 1, endLocation, endTangent, ESplineCoordinateSpace::Local);
		splineMesh->SetStartAndEnd(startLocation, startTangent, endLocation, endTangent, true);
		splineMesh->SetMaterial(0, SplineMeshMaterial);
		splineMesh->Activate(true);
		splineMesh->UpdateMesh();

		// splineMesh->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
		// splineMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		splineMesh->SetCollisionProfileName(FName("SplineArm"));
		splineMesh->SetCollisionEnabled(collisionType);
		splineMesh->OnComponentBeginOverlap.AddDynamic(this, &UArmSplineComponent::OnOverlapBegin);
		splineMesh->OnComponentEndOverlap.AddDynamic(this, &UArmSplineComponent::OnOverlapEnd);
		
		SplineMeshes.Add(splineMesh);
	}

	TSet handComponentSet = PlayerActor->GetComponents();
	for (auto it : handComponentSet)
	{
		FString name = it->GetName();
		if (name == "ArmComponent")
			HandTargetComponent = Cast<USkeletalMeshComponent>(it);
		else if (name == "HandComponent")
			RealHandComponent = Cast<USkeletalMeshComponent>(it);
	}
}


// Called every frame
void UArmSplineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	FVector playerPos = Cast<ACharacter>(PlayerActor)->GetMesh()->GetSocketLocation("ArmSocket");
	
	// doesn't work
	FVector realHandPos = RealHandComponent->GetComponentLocation();
	FVector handTargetPos = HandTargetComponent->GetComponentLocation();

	bool isVisable = FVector::Dist(playerPos, realHandPos) >= HideArmThreshold;
	if (!isVisable)
	{
		for (int32 i = 0; i < SplineMeshes.Num(); i++)
		{
			SplineMeshes[i]->SetVisibility(false);
			//SplineMeshes[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return;
	}

	LowerSamplePoint = FMath::Lerp(realHandPos, handTargetPos, LowerPointDeviation);
	UpperSamplePoint = FMath::Lerp(realHandPos, handTargetPos, UpperPointDeviation);

	LowerSamplePoint = FMath::Lerp(playerPos, LowerSamplePoint, LowerDistancePercentage);
	UpperSamplePoint = FMath::Lerp(playerPos, UpperSamplePoint, UpperDistancePercentage);

	float tValue = 0.0f;
	float tIncrement = (1.0f / (float)SplinePointCount);
	for (int32 i = 0; i < SplinePointCount; i++)
	{
		FVector pos = CalculateCurvePoint(tValue, playerPos, LowerSamplePoint, UpperSamplePoint, realHandPos);
		Spline->SetLocationAtSplinePoint(i, pos, ESplineCoordinateSpace::World);
		tValue += tIncrement;
	}

	// SplineMesh
	for (int32 i = 0; i < SplinePointCount - 1; i++)
	{
		USplineMeshComponent* splineMesh = SplineMeshes[i];

		FVector startLocation, startTangent, endLocation, endTangent;
		Spline->GetLocationAndTangentAtSplinePoint(i, startLocation, startTangent, ESplineCoordinateSpace::World);
		Spline->GetLocationAndTangentAtSplinePoint(i + 1, endLocation, endTangent, ESplineCoordinateSpace::World);
		splineMesh->SetStartAndEnd(startLocation, startTangent, endLocation, endTangent, true);
		// splineMesh->bNeverDistanceCull = true;
		// splineMesh->Activate(true);
		// splineMesh->SetVisibility(true);

		// splineMesh->bAllowCullDistanceVolume = false;
		// splineMesh->bMeshDirty = true;
	}
}


FVector UArmSplineComponent::CalculateCurvePoint(float tValue, FVector position0, FVector position1, FVector position2, FVector position3)
{
	return (FMath::Pow((1 - tValue), 3.0f) * position0) +
		(3 * FMath::Pow((1 - tValue), 2.0f) * tValue * position1) +
		(3 * (1 - tValue) * FMath::Pow(tValue, 2.0f) * position2) +
		(FMath::Pow(tValue, 3.0f) * position3);
}

UNiagaraSystem* UArmSplineComponent::GetArmHitParticleEffect() const
{
	return ArmHitParticleEffect;
}

void UArmSplineComponent::OnEnemyOverlaped_Implementation(AEnemyBase* enemy)
{
	enemy->TakeDamage(ArmHitDamage, FPointDamageEvent(), PlayerActor->GetInstigatorController(), PlayerActor);
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
	// may use a cast later
	if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass())
		&& OtherActor && IGrabableInterface::Execute_CanBeGrabbed(OtherActor))
	{
		// Arm Hit
		if(AEnemyBase* enemy = Cast<AEnemyBase>(OtherActor))
		{
			if(!EnemyRecord.Contains(enemy))
			{
				EnemyRecord.Add(enemy, 1);
				OnEnemyOverlaped(enemy);
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

				if(EnemyRecord[enemy] < 0)
					UE_LOG(LogTemp, Error, TEXT("[SplineArm] EnemyRecord became negative!"));
				if(EnemyRecord[enemy] == 0)
					EnemyRecord.Remove(enemy);
			}
		}
	}
}
