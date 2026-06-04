#pragma once

#include "CoreMinimal.h"
#include "GardenBootstrapTypes.generated.h"

/**
 * Ordered phases the world bootstrap runs through when the player presses "Start".
 * Each phase is deterministic given the world seed, so a seed reproduces a world 1:1.
 */
UENUM(BlueprintType)
enum class EGardenBootstrapPhase : uint8
{
	Idle            UMETA(DisplayName = "Idle"),
	Seeding         UMETA(DisplayName = "Seeding RNG"),
	Terrain         UMETA(DisplayName = "Terrain & Biomes"),
	Atmosphere      UMETA(DisplayName = "Atmosphere & Lighting"),
	Flora           UMETA(DisplayName = "Flora & Static Objects"),
	Items           UMETA(DisplayName = "Interactive Items & Food"),
	Fauna           UMETA(DisplayName = "Ambient Fauna"),
	Species         UMETA(DisplayName = "Garden Species (Creatures)"),
	Cognition       UMETA(DisplayName = "Cognition / ML Warmup"),
	Finalize        UMETA(DisplayName = "Finalize"),
	Complete        UMETA(DisplayName = "Complete")
};

/**
 * Top-level configuration consumed by the bootstrap. In a shipped build this is
 * filled from a UGardenWorldDataAsset; exposed here so the GameMode can drive it.
 */
USTRUCT(BlueprintType)
struct FGardenBootstrapConfig
{
	GENERATED_BODY()

	/** Master seed. 0 = pick a random seed at runtime and log it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bootstrap")
	int32 WorldSeed = 0;

	/** Horizontal extent of the side-view world, in world units. The camera pans across this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bootstrap", meta = (ClampMin = "2048"))
	float WorldWidth = 65536.f;

	/** Vertical extent of the playfield, in world units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bootstrap", meta = (ClampMin = "1024"))
	float WorldHeight = 8192.f;

	/** Number of Garden Species to spawn at startup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bootstrap", meta = (ClampMin = "0"))
	int32 InitialSpeciesCount = 6;

	/** Density multiplier for procedurally placed flora/items (1.0 = default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bootstrap", meta = (ClampMin = "0.0"))
	float ObjectDensity = 1.0f;

	/** If true, the bootstrap spreads work across frames to keep the loading screen responsive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bootstrap")
	bool bTimeSliced = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBootstrapPhaseChanged, EGardenBootstrapPhase, Phase, float, Progress01);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBootstrapComplete);
