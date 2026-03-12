#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "HealthComponent.generated.h"

DECLARE_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageReceived, AActor*, instigator, float, damageAmount, float, currentHealth, float, maxHealth);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FLOWSLAYER_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UHealthComponent();

	UFUNCTION(BlueprintPure)
	bool IsDead() const { return CurrentHealth <= 0.f; }

	UFUNCTION(BlueprintPure)
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure)
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure)
	float GetHealthRatio() const { return CurrentHealth / MaxHealth; }

	void DisplayLifeBar(bool bDisplay) { LifeBarWidget->SetVisibility(bDisplay); }

	void ReceiveDamage(float damageAmount, AActor* instigator);

	/** Executed when owning actor dies */
	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable)
	FOnDamageReceived OnDamageReceived;

protected:

	virtual void BeginPlay() override;

	/** Life bar widget ui */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* LifeBarWidget{ nullptr };

	/*
	* Make the player not take any damage
	* FOR DUBUG / TESTING ONLY
	*/
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bInvincibility{ false };

private:

	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth{ 100.f };

	float CurrentHealth{ MaxHealth };

	/* Initialize directly the blueprint variable "OwningEnemy" from this method
	* Used for now because there's no way to directly a direct reference to FSEnemy in widget blueprint
	*/
	void InitializeLifeBarWidgetRef();
};
