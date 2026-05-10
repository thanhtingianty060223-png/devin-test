#include "ReadyOrNotWeaponAnimData.h"
#include "ReadyOrNot.h"

#define DEFAULT_FLASHBANG_ANIMATION	TEXT("AnimMontage'/Game/ReadyOrNot/Animations/FP/Gamefeel/base_male/Montages/Flashbang_Affected_start_w_loop_Montage.Flashbang_Affected_start_w_loop_Montage'")
#define DEFAULT_GAS_ANIMATION		TEXT("AnimMontage'/Game/ReadyOrNot/Animations/FP/Gamefeel/base_male/Montages/CS_Gas_Affected_start_w_loop_Montage.CS_Gas_Affected_start_w_loop_Montage'")
#define DEFAULT_STING_ANIMATION		TEXT("AnimMontage'/Game/ReadyOrNot/Animations/FP/Gamefeel/base_male/Montages/Stinger_Affected_start_w_loop_Montage.Stinger_Affected_start_w_loop_Montage'")
#define DEFAULT_TASER_ANIMATION		TEXT("AnimMontage'/Game/ReadyOrNot/Animations/FP/Gamefeel/base_male/Montages/Taser_Affected_start_w_loop_Montage.Taser_Affected_start_w_loop_Montage'")

#define DEFAULT_FLASHBANG_END_ANIMATION	TEXT("AnimMontage'/Game/ReadyOrNot/Animations/FP/Gamefeel/base_male/Montages/Flashbang_Affected_end_Montage.Flashbang_Affected_end_Montage'")
#define DEFAULT_GAS_END_ANIMATION		TEXT("AnimMontage'/Game/ReadyOrNot/Animations/FP/Gamefeel/base_male/Montages/CS_Gas_Affected_end_Montage.CS_Gas_Affected_end_Montage'")
#define DEFAULT_STING_END_ANIMATION		TEXT("AnimMontage'/Game/ReadyOrNot/Animations/FP/Gamefeel/base_male/Montages/Stinger_Affected_end_Montage.Stinger_Affected_end_Montage'")
#define DEFAULT_TASER_END_ANIMATION		TEXT("AnimMontage'/Game/ReadyOrNot/Animations/FP/Gamefeel/base_male/Montages/Taser_Affected_end_Montage.Taser_Affected_end_Montage'")


UReadyOrNotWeaponAnimData::UReadyOrNotWeaponAnimData()
{
	IFMODStudioModule::Get();

	ConstructorHelpers::FObjectFinder<UAnimMontage> ReactToFlashRef(DEFAULT_FLASHBANG_ANIMATION);
	ReactToFlash.Body_FP = ReactToFlashRef.Object;

	ConstructorHelpers::FObjectFinder<UAnimMontage> ReactToGasRef(DEFAULT_GAS_ANIMATION);
	ReactToGas.Body_FP = ReactToGasRef.Object;

	ConstructorHelpers::FObjectFinder<UAnimMontage> ReactToStingRef(DEFAULT_STING_ANIMATION);
	ReactToSting.Body_FP = ReactToStingRef.Object;

	ConstructorHelpers::FObjectFinder<UAnimMontage> ReactToTaserRef(DEFAULT_TASER_ANIMATION);
	ReactToTaser.Body_FP = ReactToTaserRef.Object;

	ReactToPepperSpray.Body_FP = ReactToGasRef.Object;

	ConstructorHelpers::FObjectFinder<UAnimMontage> ReactToFlashEndRef(DEFAULT_FLASHBANG_END_ANIMATION);
	ReactToFlash_End.Body_FP = ReactToFlashEndRef.Object;

	ConstructorHelpers::FObjectFinder<UAnimMontage> ReactToGasEndRef(DEFAULT_GAS_END_ANIMATION);
	ReactToGas_End.Body_FP = ReactToGasEndRef.Object;

	ConstructorHelpers::FObjectFinder<UAnimMontage> ReactToStingEndRef(DEFAULT_STING_END_ANIMATION);
	ReactToSting_End.Body_FP = ReactToStingEndRef.Object;

	ConstructorHelpers::FObjectFinder<UAnimMontage> ReactToTaserEndRef(DEFAULT_TASER_END_ANIMATION);
	ReactToTaser_End.Body_FP = ReactToTaserEndRef.Object;

	ReactToPepperSpray_End.Body_FP = ReactToGasEndRef.Object;
}