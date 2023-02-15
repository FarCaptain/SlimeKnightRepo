// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/GrabableEnemy.h"
#include "PlayerCharacter/RCTCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

// Sets default values
AGrabableEnemy::AGrabableEnemy()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGrabableEnemy::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(false);
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
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Explode Damage"));
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
		SetActorTickEnabled(true);
		//PrimaryActorTick.bCanEverTick = true;
		if (character->GetDevourDamageInterval() < 0.1)
		{
			SetActorTickInterval(0);
			//PrimaryActorTick.TickInterval = 0;
		}
		else
		{
			SetActorTickInterval(character->GetDevourDamageInterval());
			//PrimaryActorTick.TickInterval = character->GetDevourDamageInterval();
		}
		character->AttachToArm(this);
		deathEvent.AddUObject(character, &ARCTCharacter::PromptForDevour);

		// Disable collision if it was disabled
		if (collisionDisabledOnGrab) {
			UMeshComponent* mesh = FindComponentByClass<UMeshComponent>();
			mesh->SetCollisionProfileName(TEXT("Grabbed"));
		}
	}
}

void AGrabableEnemy::LetGo_Implementation(ARCTCharacter* character)
{
	if (grabbingCharacter != nullptr && grabbingCharacter == character)
	{
		grabbingCharacter = nullptr;
		SetActorTickEnabled(false);
		//PrimaryActorTick.bCanEverTick = false;
		FDetachmentTransformRules rules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, true);
		DetachFromActor(rules);
		deathEvent.RemoveAll(grabbingCharacter);

		// Enable collision if it was disabled
		if (collisionDisabledOnGrab) {
			UMeshComponent* mesh = FindComponentByClass<UMeshComponent>();
			mesh->SetCollisionProfileName(TEXT("BlockAll"));
		}

		FTimerHandle handle;
		
		auto player = Cast<ARCTCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (player != nullptr)
		{
			GetWorldTimerManager().SetTimer(handle, this, &AGrabableEnemy::Explode, player->GetThrowTimeToExplode(), false);
		}
		else
		{
			GetWorldTimerManager().SetTimer(handle, this, &AGrabableEnemy::Explode, throwTimeToExplode, false);
		}
	}
}

bool AGrabableEnemy::CanBeGrabbed_Implementation()
{
	return grabbingCharacter == nullptr;
}

void AGrabableEnemy::Explode()
{
	auto player = Cast<ARCTCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if(player != nullptr)
	{
		// TODO: explosionDmg and knockback force to player
		float explosionDmg = player->GetThrowExplosionDamage();
		TArray<AActor*> ignoreActors = { this, player };
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