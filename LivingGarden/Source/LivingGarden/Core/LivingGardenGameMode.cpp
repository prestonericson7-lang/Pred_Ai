#include "Core/LivingGardenGameMode.h"
#include "Core/LivingGardenPlayerController.h"
#include "Core/GardenObserverPawn.h"
#include "Bootstrap/WorldBootstrapSubsystem.h"
#include "LivingGarden.h"
#include "Engine/World.h"

ALivingGardenGameMode::ALivingGardenGameMode()
{
	// The player is a free-panning side-view observer (a "hand"), not an avatar.
	DefaultPawnClass = AGardenObserverPawn::StaticClass();
	PlayerControllerClass = ALivingGardenPlayerController::StaticClass();
}

void ALivingGardenGameMode::StartPlay()
{
	Super::StartPlay();

	if (bAutoStart)
	{
		StartLivingGarden();
	}
}

void ALivingGardenGameMode::StartLivingGarden()
{
	UWorldBootstrapSubsystem* Bootstrap = GetBootstrap();
	if (!Bootstrap)
	{
		UE_LOG(LogLivingGarden, Error, TEXT("No WorldBootstrapSubsystem available; cannot start."));
		return;
	}

	if (Bootstrap->GetCurrentPhase() != EGardenBootstrapPhase::Idle)
	{
		UE_LOG(LogLivingGarden, Warning, TEXT("StartLivingGarden ignored; bootstrap already in progress or complete."));
		return;
	}

	Bootstrap->OnBootstrapComplete.AddDynamic(this, &ALivingGardenGameMode::HandleBootstrapComplete);
	Bootstrap->OnPhaseChanged.AddDynamic(this, &ALivingGardenGameMode::HandleBootstrapPhaseChanged);

	Bootstrap->BeginBootstrap(BootstrapConfig);
}

void ALivingGardenGameMode::HandleBootstrapPhaseChanged(EGardenBootstrapPhase Phase, float Progress01)
{
	UE_LOG(LogLivingGarden, Verbose, TEXT("Bootstrap phase %d (%.0f%%)"),
		static_cast<int32>(Phase), Progress01 * 100.f);
}

void ALivingGardenGameMode::HandleBootstrapComplete()
{
	UE_LOG(LogLivingGarden, Display, TEXT("World ready. Living Garden simulation is now live."));
	// Subsequent steps: dismiss the loading screen, enable creature ticking,
	// and hand control of the observer camera to the player here.
}

UWorldBootstrapSubsystem* ALivingGardenGameMode::GetBootstrap() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UWorldBootstrapSubsystem>() : nullptr;
}
