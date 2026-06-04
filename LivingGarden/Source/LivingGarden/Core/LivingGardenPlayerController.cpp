#include "Core/LivingGardenPlayerController.h"
#include "Core/GardenObserverPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

ALivingGardenPlayerController::ALivingGardenPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ALivingGardenPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (ObserverMappingContext)
		{
			Subsystem->AddMappingContext(ObserverMappingContext, 0);
		}
	}
}

void ALivingGardenPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PanAction)
		{
			EIC->BindAction(PanAction, ETriggerEvent::Triggered, this, &ALivingGardenPlayerController::OnPan);
		}
	}
}

void ALivingGardenPlayerController::OnPan(const FInputActionValue& Value)
{
	if (AGardenObserverPawn* Observer = Cast<AGardenObserverPawn>(GetPawn()))
	{
		Observer->PanHorizontal(Value.Get<float>());
	}
}
