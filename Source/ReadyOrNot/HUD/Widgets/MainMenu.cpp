// Void Interactive, 2020

#include "HUD/Widgets/MainMenu.h"

#include "CommonUISubsystemBase.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "lib/GameFeatureLibrary.h"

void UMainMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	bEnableFindSessionCOOPButton = true;
	bEnableFindSessionPVPButton = true;
}

void UMainMenu::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!GetWorld())
		return;

	const ESlateVisibility LoginVisibility = IsLoggedIn() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	
	FindCOOPSessionBtn->SetIsEnabled(CanFindSession(true));
	FindPVPSessionBtn->SetIsEnabled(CanFindSession(false));

#ifndef RON_PVP_ENABLED
	FindPVPSessionBtn->SetVisibility(ESlateVisibility::Collapsed);
#endif

	PlayPublicLobbyBtn->SetIsEnabled(CanPlayPublicLobby());
	PlayPublicLobbyBtn->SetVisibility(LoginVisibility);
	
	PlayFriendsOnlyBtn->SetVisibility(LoginVisibility);
	
	if (const UReadyOrNotGameInstance* GameInstance = GetWorld()->GetGameInstance<UReadyOrNotGameInstance>())
	{
		const ELoginState CurrentLoginState = (ELoginState)GameInstance->GetBackendState();
		
		if (BackendConnectionStatus.Find(CurrentLoginState))
		{
			Txt_BackEndConnection->SetText(BackendConnectionStatus[CurrentLoginState]);
		}

		if (BackendConnectionStatusColor.Find(CurrentLoginState))
		{
			Txt_BackEndConnection->SetColorAndOpacity(BackendConnectionStatusColor[CurrentLoginState]);
		}

		Txt_PublicLobbyCooldown->SetText(GetPublicLobbyCooldown());
		Txt_PublicLobbyCooldown->SetVisibility(CanPlayPublicLobby() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

FText UMainMenu::GetVersion() const
{
	return FText::FromString(UBpGameplayHelperLib::GetProjectName() + " " + UBpGameplayHelperLib::GetProjectVersion());
}

bool UMainMenu::IsLoggedIn() const
{
	if(UGameFeatureLibrary::IsGameDemo())
		return false;
#if WITH_EDITOR
	return true;
#endif
	if (GetWorld() && GetWorld()->GetGameInstance<UReadyOrNotGameInstance>())
		return GetWorld()->GetGameInstance<UReadyOrNotGameInstance>()->IsLoggedIntoBackend();

	return false;
}

bool UMainMenu::CanFindSession(bool bCOOP) const
{
	return ((bCOOP && bEnableFindSessionCOOPButton) || (!bCOOP && bEnableFindSessionPVPButton)) && (UBpGameplayHelperLib::IsEditorBuild() || IsLoggedIn());
}

FText UMainMenu::GetBackEndConnectionStatus(const ELoginState LoginState) const
{
	return BackendConnectionStatus[LoginState];
}

FText UMainMenu::GetPublicLobbyCooldown() const
{
	float SecondsRemaining = 0.0f;
	UBpGameplayHelperLib::IsInPublicLobbyCooldown(SecondsRemaining);
	
	return FText::FromString("Public Lobby Cooldown (" + UBpGameplayHelperLib::ConvertFloatToStringMinutes(SecondsRemaining) + ")");
}

bool UMainMenu::CanPlayPublicLobby() const
{
	if(UGameFeatureLibrary::IsGameDemo())
		return false;
	
	float SecondsRemaining = 0.0f;
	UBpGameplayHelperLib::IsInPublicLobbyCooldown(SecondsRemaining);
	
	return SecondsRemaining <= 0.0f;
}

void UMainMenu::OpenModMenu()
{
	//UCommonUISubsystemBase::bDisableVirtualAccept = true; // causes compile error "bDisableVirtaulAccept" does not exist
}

void UMainMenu::CloseModMenu()
{
	//UCommonUISubsystemBase::bDisableVirtualAccept = false; // causes compile error "bDisableVirtaulAccept" does not exist
}
