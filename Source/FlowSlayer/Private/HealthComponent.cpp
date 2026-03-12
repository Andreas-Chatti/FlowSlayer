#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

    LifeBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeBarWidget"));
    LifeBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
    LifeBarWidget->SetDrawSize(FVector2D(100.f, 15.f));
    LifeBarWidget->SetVisibility(false);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

    // Register and attach life bar to the owner's mesh
    // WidgetComponent created inside an UActorComponent constructor is not auto-registered
    // as an actor component — it has no World reference and InitWidget() is never called.
    // We must register it explicitly so GetUserWidgetObject() returns a valid widget.
    ACharacter* ownerCharacter{ Cast<ACharacter>(GetOwner()) };
    if (ownerCharacter)
    {
        if (!LifeBarWidget->IsRegistered())
            LifeBarWidget->RegisterComponent();

        LifeBarWidget->AttachToComponent(ownerCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
        LifeBarWidget->InitWidget();
    }

    InitializeLifeBarWidgetRef();
}

void UHealthComponent::ReceiveDamage(float damageAmount, AActor* instigator)
{
    if (IsDead() || bInvincibility)
        return;

    CurrentHealth -= damageAmount;

    LifeBarWidget->SetVisibility(true);

    OnDamageReceived.Broadcast(instigator, damageAmount, CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.f)
    {
        OnDeath.ExecuteIfBound();
        LifeBarWidget->SetVisibility(false);
    }
}

void UHealthComponent::InitializeLifeBarWidgetRef()
{
    if (!LifeBarWidget)
        return;

    UUserWidget* Widget{ LifeBarWidget->GetUserWidgetObject() };
    if (!Widget)
        return;

    // Set the OwningActor property on the widget
    FObjectProperty* Prop{ FindFProperty<FObjectProperty>(Widget->GetClass(), TEXT("OwningActor")) };
    if (!Prop)
        return;

    void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Widget);
    Prop->SetObjectPropertyValue(ValuePtr, GetOwner());
}