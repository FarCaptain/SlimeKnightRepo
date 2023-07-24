// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/GrabableEnemy.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "PlayerCharacter/RCTCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"

AGrabableEnemy::AGrabableEnemy()
{
 	// Need to keep the tick for the collision fix
	PrimaryActorTick.bCanEverTick = true;

	UCapsuleComponent* capsule = FindComponentByClass<UCapsuleComponent>();

	stopSlidingVelocityThreshold = 10.0f;
	
	originalCollisionProfileName = GetCapsuleComponent()->GetCollisionProfileName();
}

// Called when the game starts or when spawned
void AGrabableEnemy::BeginPlay()
{
	Super::BeginPlay();
}

float AGrabableEnemy::TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser)
{
	if (damageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		auto player = Cast<ARCTCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (player == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to find player in AGrabableEnemy::TakeDamage"));
			return 0;
		}
		TArray<UStaticMeshComponent*> staticMeshComponents;
		GetComponents<UStaticMeshComponent>(staticMeshComponents);
		for (auto& mesh : staticMeshComponents)
		{
			mesh->AddRadialImpulse(damageCauser->GetActorLocation(), player->GetThrowExplosionRadius(), player->GetThrowKnockbackImpulse(), ERadialImpulseFalloff::RIF_Constant);
		}
	}
	return Super::TakeDamage(damageAmount, damageEvent, eventInstigator, damageCauser);
}


// Called every frame
void AGrabableEnemy::Tick(float DeltaTime)
{

	Super::Tick(DeltaTime);

	if (grabbingCharacter != nullptr)
	{
		if(health > 0)
		{
			// Subtract Health
			float modifier = 0.f;
			if (PrimaryActorTick.TickGroup == 0)
			{
				modifier -= grabbingCharacter->GetDevourDamage() * DeltaTime;
			}
			else
			{
				modifier -= grabbingCharacter->GetDevourDamage();
			}
			grabbingCharacter->AbsorbAfterDamaging(-modifier);
			UGameplayStatics::ApplyDamage(this, 0, GetController(), this, UDamageType::StaticClass());
			ModifyHealth(modifier);
		}
	}
}

// Called to bind functionality to input
void AGrabableEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AGrabableEnemy::Grab_Implementation(ARCTCharacter* character)
{
	if (character != nullptr && grabbingCharacter == nullptr)
	{
		grabbingCharacter = character;

		/* we need tick right now
		if (character->GetDevourDamageInterval() < 0.1)
		{
			SetActorTickInterval(0);
		}
		else
		{
			SetActorTickInterval(character->GetDevourDamageInterval());
		}
		*/

		character->AttachToArm(this);
		hpZeroEvent.AddUniqueDynamic(character, &ARCTCharacter::PromptForDevour);

		// Stop BT
		AAIController* controller = Cast<AAIController>(GetController());
		if(controller)
		{
			controller->GetBrainComponent()->PauseLogic("Grabbed");
		}
		
		GetCapsuleComponent()->SetSimulatePhysics(false);

		// Disable collision if it was disabled
		if (collisionDisabledOnGrab) {
			GetCapsuleComponent()->SetCollisionProfileName(TEXT("Grabbed"));
		}
	}
}

void AGrabableEnemy::LetGo_Implementation(ARCTCharacter* character)
{
	if (grabbingCharacter != nullptr && grabbingCharacter == character)
	{
		grabbingCharacter = nullptr;
		FDetachmentTransformRules rules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, true);
		DetachFromActor(rules);
		deathEvent.RemoveAll(grabbingCharacter);
		character->EndPromptForDevour();
		
		// Enable collision if it was disabled
		if (collisionDisabledOnGrab) {
			GetCapsuleComponent()->SetCollisionProfileName(originalCollisionProfileName);
		}

		FTimerHandle handle;
		bExploded = false;

		AAIController* controller = Cast<AAIController>(GetController());
		if(controller)
		{
			controller->GetBrainComponent()->ResumeLogic("LetGo");
		}
		
		// Throwing
		GetCapsuleComponent()->OnComponentHit.AddUniqueDynamic(this, &AGrabableEnemy::OverlapExplode);
		GetCapsuleComponent()->SetSimulatePhysics(true);

		GetWorldTimerManager().SetTimer(thrownSlideHandle, this, &AGrabableEnemy::ThrownSlide, 0.1f, true);

		auto player = Cast<ARCTCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (player != nullptr)
		{
			GetWorldTimerManager().SetTimer(handle, this, &AGrabableEnemy::Explode, player->GetThrowTimeToExplode(), false);
		}
		else
		{
			GetWorldTimerManager().SetTimer(handle, this, &AGrabableEnemy::Explode, throwTimeToExplode, false);
		}
		// AddOverlapBegin
		
		//Post Throw
	}
}

void AGrabableEnemy::HitByFist()
{
	if(!punchable)
	{
		return;
	}

	GetCapsuleComponent()->SetSimulatePhysics(true);
	GetWorldTimerManager().SetTimer(thrownSlideHandle, this, &AGrabableEnemy::ThrownSlide, 0.1f, true);
}

bool AGrabableEnemy::CanBeGrabbed_Implementation()
{
	bool isGrabbed = (grabbingCharacter != nullptr);
	return !isGrabbed && !bIsDead && bIsGrabbable;
}

bool AGrabableEnemy::IsGrabbed_Implementation()
{
	return grabbingCharacter != nullptr;
}

void AGrabableEnemy::SetHilighting_Implementation(bool bIfHighlight)
{
	GetMesh()->SetScalarParameterValueOnMaterials("GrabToggle", bIfHighlight);
}

void AGrabableEnemy::ThrownSlide()
{
	if(GetVelocity().Length() <= stopSlidingVelocityThreshold)
	{
		UCapsuleComponent* capsule = FindComponentByClass<UCapsuleComponent>();
		capsule->SetSimulatePhysics(false);
		GetWorldTimerManager().ClearTimer(thrownSlideHandle);
	}
}

void AGrabableEnemy::OverlapExplode(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if(OtherActor && OtherActor->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass()))
	{
		Explode();
	}
}

void AGrabableEnemy::Explode()
{
	if(bExploded)
		return;

	UCapsuleComponent* capsule = FindComponentByClass<UCapsuleComponent>();
	capsule->OnComponentHit.RemoveAll(this);
	
	auto player = Cast<ARCTCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if(player != nullptr)
	{
		// TODO: explosionDmg and knockback force to player
		float explosionDmg = player->GetThrowExplosionDamage();
		TArray<AActor*> ignoreActors = { player };
		if(player->GetIfApplyExplosionDamageToInstigator() == false)
		{
			ignoreActors.Add(this);
		}

		auto particleEffect = player->GetThrowExplosionParticleEffect();
		if (particleEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), particleEffect,
				GetActorLocation());
		}
		UGameplayStatics::ApplyRadialDamage(GetWorld(), explosionDmg, GetActorLocation(),
			player->GetThrowExplosionRadius(), UDamageType::StaticClass(), ignoreActors, this);
		if (health <= 0)
		{
			Destroy();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Player Not found in AGrabableEnemy::Explode()"));
	}
}

void AGrabableEnemy::FellOutOfWorld(const UDamageType& dmgType)
{
	if (health > 0) 
	{
		hpZeroEvent.Broadcast();
	}

	Explode();
	//Check health just to be safe
	//if (health > 0) 
	//{
		//deathEvent.Broadcast();
		//deathEvent.Clear();
	//}

	Super::FellOutOfWorld(dmgType);
}
