#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GardenObserverPawn.generated.h"

class UCameraComponent;
class USceneComponent;

/**
 * AGardenObserverPawn
 *
 * The player's presence in Living Garden is a disembodied observer that pans the
 * 2D side-view world left-to-right (and within vertical limits), matching the
 * classic side-scrolling life-sim camera. It is not a character: the player
 * watches and interacts with the world and the Garden Species, never controls one.
 *
 * An orthographic camera looking down +X keeps the world flat on the X/Z plane,
 * exactly like a 2D side view, while still rendering Paper2D sprites.
 */
UCLASS()
class LIVINGGARDEN_API AGardenObserverPawn : public APawn
{
	GENERATED_BODY()

public:
	AGardenObserverPawn();

	/** Horizontal pan speed in world units/second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer")
	float PanSpeed = 2000.f;

	/** Min/max horizontal position the camera may reach. Set from the world width at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer")
	FVector2D PanBoundsX = FVector2D(0.f, 65536.f);

	/** Orthographic width controlling how much of the world is visible at once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer")
	float ViewWidth = 3200.f;

	/** Move the camera horizontally; +1 pans right, -1 pans left. */
	UFUNCTION(BlueprintCallable, Category = "Observer")
	void PanHorizontal(float AxisValue);

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Observer")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Observer")
	TObjectPtr<UCameraComponent> Camera;

private:
	float PendingPan = 0.f;
};
