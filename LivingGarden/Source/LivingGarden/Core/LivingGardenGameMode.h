#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Bootstrap/GardenBootstrapTypes.h"
#include "LivingGardenGameMode.generated.h"

class UWorldBootstrapSubsystem;

/**
 * ALivingGardenGameMode
 *
 * Owns the lifecycle of a Living Garden session. On StartPlay it asks the
 * UWorldBootstrapSubsystem to procedurally build the entire 2D side-view world,
 * then transitions into the live life-simulation once the build completes.
 *
 * No gameplay objects are hand-placed in the level: the only thing the level needs
 * is this GameMode (set as the default) and an empty playspace.
 */
UCLASS()
class LIVINGGARDEN_API ALivingGardenGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALivingGardenGameMode();

	virtual void StartPlay() override;

	/** Configuration handed to the bootstrap. Editable per-level / per-difficulty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Living Garden")
	FGardenBootstrapConfig BootstrapConfig;

	/**
	 * When true, the world auto-bootstraps on StartPlay. Set false if a front-end
	 * menu should call StartLivingGarden() in response to the player's "Start" button.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Living Garden")
	bool bAutoStart = true;

	/** Entry point wired to the "Start" button. Kicks off procedural generation. */
	UFUNCTION(BlueprintCallable, Category = "Living Garden")
	void StartLivingGarden();

protected:
	/** Bound to the bootstrap subsystem; runs once the world is fully generated. */
	UFUNCTION()
	void HandleBootstrapComplete();

	UFUNCTION()
	void HandleBootstrapPhaseChanged(EGardenBootstrapPhase Phase, float Progress01);

private:
	UWorldBootstrapSubsystem* GetBootstrap() const;
};
