#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LivingGardenPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * ALivingGardenPlayerController
 *
 * Drives the observer camera and the player's point-and-click interaction with the
 * world (selecting Garden Species, picking up / moving items, etc.). Uses Enhanced
 * Input. Concrete input assets are created in the editor in a later step; the
 * properties below are wired in Blueprint-derived class BP_LivingGardenPC.
 */
UCLASS()
class LIVINGGARDEN_API ALivingGardenPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALivingGardenPlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> ObserverMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> PanAction;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void OnPan(const FInputActionValue& Value);
};
