#include "Core/GardenObserverPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

AGardenObserverPawn::AGardenObserverPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);

	// Orthographic side view: look along +X so sprites live on the X(horizontal)/Z(vertical) plane.
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(ViewWidth);
	Camera->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
}

void AGardenObserverPawn::PanHorizontal(float AxisValue)
{
	PendingPan = AxisValue;
}

void AGardenObserverPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Camera->SetOrthoWidth(ViewWidth);

	if (!FMath::IsNearlyZero(PendingPan))
	{
		FVector Location = GetActorLocation();
		Location.Y += PendingPan * PanSpeed * DeltaSeconds; // pan along Y (screen-horizontal)
		Location.Y = FMath::Clamp(Location.Y, PanBoundsX.X, PanBoundsX.Y);
		SetActorLocation(Location);
	}

	PendingPan = 0.f;
}
