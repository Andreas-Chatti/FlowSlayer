#include "AnimNotifyState_AnimCancelWindow.h"

UAnimNotifyState_AnimCancelWindow::UAnimNotifyState_AnimCancelWindow()
{
	NotifyColor = FColor::Green;
	bShouldFireInEditor = false;
}

void UAnimNotifyState_AnimCancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner{ MeshComp->GetOwner() };
	FSCharacter = Cast<AFlowSlayerCharacter>(Owner);
	if (!FSCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("[UAnimNotifyState_AnimCancelWindow] ERROR: FSCharacter ref is NULL or INVALID !"));
		return;
	}

	APlayerController* PC{ FSCharacter->GetInputManagerComponent()->GetPlayerController() };
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[UAnimNotifyState_AnimCancelWindow] ERROR: PlayerController is NULL or INVALID !"));
		return;
	}

	EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("[UAnimNotifyState_AnimCancelWindow] ERROR: EnhancedInputComponent is NULL or INVALID !"));
		return;
	}

	if (ShouldBind(EAnimCancelWindowActionType::Dash))
	{
		if (DashAction)
		{
			FEnhancedInputActionEventBinding& binding{ EIC->BindAction(DashAction, ETriggerEvent::Started, this, &UAnimNotifyState_AnimCancelWindow::HandleDashCancelInput) };
			DashBindingHandle = binding.GetHandle();
		}
		else
			UE_LOG(LogTemp, Error, TEXT("[UAnimNotifyState_AnimCancelWindow] ERROR: DashAction is NULL or INVALID !"));
	}

	if (ShouldBind(EAnimCancelWindowActionType::Move))
	{
		if (MoveAction)
		{
			FEnhancedInputActionEventBinding& binding{ EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UAnimNotifyState_AnimCancelWindow::HandleMoveCancelInput) };
			MoveBindingHandle = binding.GetHandle();
		}
		else
			UE_LOG(LogTemp, Error, TEXT("[UAnimNotifyState_AnimCancelWindow] ERROR: MoveAction is NULL or INVALID !"));
	}

	if (ShouldBind(EAnimCancelWindowActionType::Guard))
	{
		if (GuardAction)
		{
			FEnhancedInputActionEventBinding& binding{ EIC->BindAction(GuardAction, ETriggerEvent::Triggered, this, &UAnimNotifyState_AnimCancelWindow::HandleGuardCancelInput) };
			GuardBindingHandle = binding.GetHandle();
		}
		else
			UE_LOG(LogTemp, Error, TEXT("[UAnimNotifyState_AnimCancelWindow] ERROR: GuardAction is NULL or INVALID !"));
	}
}

void UAnimNotifyState_AnimCancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (EIC)
	{
		for (auto* bindingHandle : { &MoveBindingHandle, &DashBindingHandle, &GuardBindingHandle })
			EIC->RemoveBindingByHandle(*bindingHandle);

		EIC = nullptr;
	}

	MoveBindingHandle = 0;
	DashBindingHandle = 0;
	GuardBindingHandle = 0;
	FSCharacter = nullptr;
	bAnimCancelTrigger = false;
}

bool UAnimNotifyState_AnimCancelWindow::ShouldBind(EAnimCancelWindowActionType Type) const
{
	if (CancelActionTrigger == EAnimCancelWindowActionType::Any)
		return true;
	if (CancelActionTrigger == EAnimCancelWindowActionType::Custom)
		return CancelActionTriggerCustom.Contains(Type);
	return CancelActionTrigger == Type;
}

void UAnimNotifyState_AnimCancelWindow::HandleMoveCancelInput()
{
	if (bAnimCancelTrigger)
		return;
	UE_LOG(LogTemp, Error, TEXT("[UAnimNotifyState_AnimCancelWindow] MoveCancelInput launched"));
	FSCharacter->OnAnimationCanceled.Broadcast(CancelBlendOutTime, MoveAction);
	bAnimCancelTrigger = true;
}

void UAnimNotifyState_AnimCancelWindow::HandleGuardCancelInput()
{
	if (bAnimCancelTrigger)
		return;
	UE_LOG(LogTemp, Error, TEXT("[UAnimNotifyState_AnimCancelWindow] GuardCancelInput launched"));
	FSCharacter->OnAnimationCanceled.Broadcast(CancelBlendOutTime, GuardAction);
	bAnimCancelTrigger = true;
}

void UAnimNotifyState_AnimCancelWindow::HandleDashCancelInput()
{
	if (bAnimCancelTrigger)
		return;

	FSCharacter->OnAnimationCanceled.Broadcast(CancelBlendOutTime, DashAction);
	bAnimCancelTrigger = true;
}
