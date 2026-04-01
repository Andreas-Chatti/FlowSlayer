#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "UpgradeData.h"
#include "HealthComponent.generated.h"

DECLARE_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeal);
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

	UFUNCTION(BlueprintPure)
	bool IsHealOnCooldown() const { return bIsHealOnCooldown; }

	/** Returns the flow cost required to use the heal skill */
	float GetHealFlowCost() const { return HealFlowCost; }

	void DisplayLifeBar(bool bDisplay) { LifeBarWidget->SetVisibility(bDisplay); }

	void ReceiveDamage(float damageAmount, AActor* instigator);

	/** Heal the owner to {MaxHealth} and plays heal animation */
	UFUNCTION(BlueprintCallable)
	void Heal();

	/** Applies upgrade effects that concern the health system (MaxHealth, HealCooldown, HealFlowCost stats) */
	UFUNCTION()
	void HandleOnUpgradeSelected(const FUpgradeData& Upgrade);

	/** Executed when owning actor dies */
	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable)
	FOnDamageReceived OnDamageReceived;

	/** Broadcast when owner successfully heals
	* Used mainly to notify and handle the animation on ABP
	*/
	UPROPERTY(BlueprintAssignable)
	FOnHeal OnHeal;

protected:

	virtual void BeginPlay() override;

	/** Life bar widget ui */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health | UI")
	UWidgetComponent* LifeBarWidget{ nullptr };

	/*
	* Make the player not take any damage
	* FOR DUBUG / TESTING ONLY
	*/
	UPROPERTY(EditAnywhere, Category = "Health | Debug")
	bool bInvincibility{ false };

	/** Flow cost of the heal skill */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health | Heal", meta = (ClampMin = "0.0"))
	float HealFlowCost{ 50.f };

	/** Cooldown of heal ability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health | Heal", meta = (ClampMin = "0.0"))
	float HealCooldown{ 5.f };

	/** TimerHandle of heal ability */
	UPROPERTY(BlueprintReadOnly)
	FTimerHandle HealCooldownTimer;

	/** Track state of heal cooldown */
	bool bIsHealOnCooldown{ false };

	/** Maximum health of the owner */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth{ 100.f };

private:

	/** Current health of the owner — clamped to [0, MaxHealth] */
	float CurrentHealth{ MaxHealth };

	/* Initialize directly the blueprint variable "OwningEnemy" from this method
	* Used for now because there's no way to directly a direct reference to FSEnemy in widget blueprint
	*/
	void InitializeLifeBarWidgetRef();
};
