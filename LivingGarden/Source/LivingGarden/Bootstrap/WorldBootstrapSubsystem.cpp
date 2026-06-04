#include "Bootstrap/WorldBootstrapSubsystem.h"
#include "LivingGarden.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UWorldBootstrapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentPhase = EGardenBootstrapPhase::Idle;
}

void UWorldBootstrapSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StepTimer);
	}
	Super::Deinitialize();
}

void UWorldBootstrapSubsystem::BeginBootstrap(const FGardenBootstrapConfig& InConfig)
{
	if (bRunning)
	{
		UE_LOG(LogLivingGarden, Warning, TEXT("BeginBootstrap called while already running. Ignored."));
		return;
	}

	Config = InConfig;

	// Resolve and lock the seed up front so the whole build is reproducible.
	const int32 ResolvedSeed = (Config.WorldSeed != 0)
		? Config.WorldSeed
		: FMath::Rand() * 2654435761u + 1; // never 0
	Config.WorldSeed = ResolvedSeed;
	RandomStream.Initialize(ResolvedSeed);

	UE_LOG(LogLivingGarden, Display,
		TEXT("Bootstrapping Living Garden | Seed=%d Width=%.0f Height=%.0f Species=%d Density=%.2f TimeSliced=%s"),
		Config.WorldSeed, Config.WorldWidth, Config.WorldHeight,
		Config.InitialSpeciesCount, Config.ObjectDensity,
		Config.bTimeSliced ? TEXT("true") : TEXT("false"));

	bRunning = true;
	SetPhase(EGardenBootstrapPhase::Seeding, 0.f);

	if (Config.bTimeSliced)
	{
		// Spread phases across frames so the loading screen stays responsive.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(StepTimer, this, &UWorldBootstrapSubsystem::AdvancePhase, 0.01f, true);
		}
	}
	else
	{
		// Build the whole world in one blocking pass.
		while (bRunning)
		{
			AdvancePhase();
		}
	}
}

void UWorldBootstrapSubsystem::AdvancePhase()
{
	if (!bRunning)
	{
		return;
	}

	const bool bPhaseDone = RunPhase(CurrentPhase);
	if (!bPhaseDone)
	{
		// Generator needs more frames (large/time-sliced work). Stay on this phase.
		return;
	}

	// Move to the next phase in declaration order.
	const uint8 Next = static_cast<uint8>(CurrentPhase) + 1;
	const EGardenBootstrapPhase NextPhase = static_cast<EGardenBootstrapPhase>(Next);

	if (NextPhase >= EGardenBootstrapPhase::Complete)
	{
		SetPhase(EGardenBootstrapPhase::Complete, 1.f);
		bRunning = false;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(StepTimer);
		}

		UE_LOG(LogLivingGarden, Display, TEXT("Living Garden bootstrap complete."));
		OnBootstrapComplete.Broadcast();
		return;
	}

	// Progress runs from Seeding(1) .. Finalize over the active phase count.
	const float Progress = static_cast<float>(Next) / static_cast<float>(EGardenBootstrapPhase::Complete);
	SetPhase(NextPhase, Progress);
}

bool UWorldBootstrapSubsystem::RunPhase(EGardenBootstrapPhase Phase)
{
	// Step 1 scaffold: each phase logs its work and reports "done" immediately.
	// Subsequent steps replace these bodies with the real generators
	// (terrain, flora, items, species, cognition). Each must pull randomness only
	// from GetStream() to preserve determinism, and may return false to request
	// additional frames when time-slicing heavy work.
	switch (Phase)
	{
	case EGardenBootstrapPhase::Seeding:
		UE_LOG(LogLivingGarden, Verbose, TEXT("[Bootstrap] RNG stream seeded."));
		return true;

	case EGardenBootstrapPhase::Terrain:
		UE_LOG(LogLivingGarden, Verbose, TEXT("[Bootstrap] Terrain & biomes (placeholder)."));
		return true;

	case EGardenBootstrapPhase::Atmosphere:
		UE_LOG(LogLivingGarden, Verbose, TEXT("[Bootstrap] Atmosphere & lighting (placeholder)."));
		return true;

	case EGardenBootstrapPhase::Flora:
		UE_LOG(LogLivingGarden, Verbose, TEXT("[Bootstrap] Flora & static objects (placeholder)."));
		return true;

	case EGardenBootstrapPhase::Items:
		UE_LOG(LogLivingGarden, Verbose, TEXT("[Bootstrap] Interactive items & food (placeholder)."));
		return true;

	case EGardenBootstrapPhase::Fauna:
		UE_LOG(LogLivingGarden, Verbose, TEXT("[Bootstrap] Ambient fauna (placeholder)."));
		return true;

	case EGardenBootstrapPhase::Species:
		UE_LOG(LogLivingGarden, Verbose, TEXT("[Bootstrap] Spawning %d Garden Species (placeholder)."), Config.InitialSpeciesCount);
		return true;

	case EGardenBootstrapPhase::Cognition:
		UE_LOG(LogLivingGarden, Verbose, TEXT("[Bootstrap] Cognition / ML warmup (placeholder)."));
		return true;

	case EGardenBootstrapPhase::Finalize:
		UE_LOG(LogLivingGarden, Verbose, TEXT("[Bootstrap] Finalize (placeholder)."));
		return true;

	default:
		return true;
	}
}

void UWorldBootstrapSubsystem::SetPhase(EGardenBootstrapPhase NewPhase, float Progress01)
{
	CurrentPhase = NewPhase;
	OnPhaseChanged.Broadcast(NewPhase, FMath::Clamp(Progress01, 0.f, 1.f));
}
