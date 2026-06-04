#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Bootstrap/GardenBootstrapTypes.h"
#include "WorldBootstrapSubsystem.generated.h"

/**
 * UWorldBootstrapSubsystem
 *
 * The single entry point for procedurally generating the entire Living Garden world
 * at runtime. Nothing in the side-view world is hand-placed in the editor: when the
 * player presses "Start", the GameMode calls BeginBootstrap(), and this subsystem runs
 * an ordered, deterministic, optionally time-sliced pipeline (see EGardenBootstrapPhase).
 *
 * Each phase delegates to a dedicated generator (terrain, flora, items, species, ...),
 * all driven from a single seeded FRandomStream so a given seed reproduces a world 1:1.
 */
UCLASS()
class LIVINGGARDEN_API UWorldBootstrapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Kicks off the full procedural build. Safe to call once per world. */
	UFUNCTION(BlueprintCallable, Category = "Living Garden|Bootstrap")
	void BeginBootstrap(const FGardenBootstrapConfig& InConfig);

	/** True once the world has finished generating and gameplay can proceed. */
	UFUNCTION(BlueprintPure, Category = "Living Garden|Bootstrap")
	bool IsWorldReady() const { return CurrentPhase == EGardenBootstrapPhase::Complete; }

	UFUNCTION(BlueprintPure, Category = "Living Garden|Bootstrap")
	EGardenBootstrapPhase GetCurrentPhase() const { return CurrentPhase; }

	/** The seeded RNG stream every generator must use, so worlds are reproducible. */
	FRandomStream& GetStream() { return RandomStream; }

	/** Fired each time the pipeline advances; great for driving a loading screen. */
	UPROPERTY(BlueprintAssignable, Category = "Living Garden|Bootstrap")
	FOnBootstrapPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Living Garden|Bootstrap")
	FOnBootstrapComplete OnBootstrapComplete;

protected:
	/** Advances the pipeline one phase. Hooked to the world timer when time-slicing. */
	void AdvancePhase();

	/** Runs the generator for a single phase. Returns true when that phase is done. */
	bool RunPhase(EGardenBootstrapPhase Phase);

	void SetPhase(EGardenBootstrapPhase NewPhase, float Progress01);

private:
	UPROPERTY()
	FGardenBootstrapConfig Config;

	EGardenBootstrapPhase CurrentPhase = EGardenBootstrapPhase::Idle;

	FRandomStream RandomStream;

	FTimerHandle StepTimer;

	bool bRunning = false;
};
