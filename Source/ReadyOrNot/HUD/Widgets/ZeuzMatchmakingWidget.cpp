// ##UE5UPGRADE## Zeus
//// Copyright Void Interactive, 2021
//
//
//#include "ZeuzMatchmakingWidget.h"
//#include "ApiMatchmaking.h"
//#include "ReadyOrNotGameInstance.h"
//#include <Kismet/GameplayStatics.h>
//#include <TimerManager.h>
//#include "Characters/ReadyOrNotPlayerController.h"
//
//void UZeuzMatchmakingWidget::ResetMatchmaking()
//{
//	MatchmakingStatus = EMatchmakingStatus::MS_None;
//	ZeuzMatchmakingStatus = FZeuzMatchMakingStatus();
//}
//
//void UZeuzMatchmakingWidget::StartMatchmaking(FString Region)
//{
	// AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
	// if (pc)
	// {
//		LastMatchmakeRegion = Region;
//
//		FZeuzMatchmakingUser MatchmakingUser;
//		MatchmakingUser.UserID = pc->PlayerState->GetUniqueId()->ToString();
//		MatchmakingUser.DisplayName = pc->PlayerState->GetPlayerName();
//
//		FZeuzMatchMakingPartyInit MatchmakingInit;
//		MatchmakingInit.Region = Region;
//		if (MatchmakingInit.Party.Num() == 0)
//		{
//			MatchmakingInit.Party.Add(MatchmakingUser);
//		}
//		MatchmakingCreateDelegate.BindUObject(this, &UZeuzMatchmakingWidget::OnZeuzMatchmakingCreate);
//		UZeuzApiMatchmaking::MatchmakingCreateparty(MatchmakingInit, MatchmakingCreateDelegate);
//	}
// 
//}
//
//void UZeuzMatchmakingWidget::StartPartyMatchmaking()
//{
//	MatchmakingStatus = EMatchmakingStatus::MS_Matchmaking;
//	/*for (TActorIterator<ASteamBeaconPlayerState> It(GetWorld()); It; ++It)
//	{
//		ASteamBeaconPlayerState* ps = *It;
//		if (ps->IsPartyLeader())
//		{
//			AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
//			if (pc)
//			{
//				pc->SetPreferredTeamUniqueNetIdOnConnection(ps->GetPlayerUniqueIdAsString());
//			}
//		}
//	}*/
//}
//
//void UZeuzMatchmakingWidget::StopPartyMatchmaking()
//{
//	MatchmakingStatus = EMatchmakingStatus::MS_Cancelled;
//}
//
//void UZeuzMatchmakingWidget::OnZeuzMatchmakingCreate(const FZeuzMatchMakingStatus& Status, FString Error)
//{
//	ZeuzMatchmakingStatus = Status;
//	if (!Error.IsEmpty())
//	{
//		MatchmakingStatus = EMatchmakingStatus::MS_Failure;
//		ZeuzMatchmakingStatus.Error = Error;
//		return;
//	}
//
//	if (Status.Error == "" && Status.State != "retry")
//	{
//		MatchmakingStatus = EMatchmakingStatus::MS_Matchmaking;
//		MatchmakingUpdateDelegate.BindUObject(this, &UZeuzMatchmakingWidget::OnZeuzMatchmakingUpdate);
//		UZeuzApiMatchmaking::MatchmakingUpdate(ZeuzMatchmakingStatus.MatchMakingId, MatchmakingUpdateDelegate);
//	}
//}
//
//void UZeuzMatchmakingWidget::OnZeuzMatchmakingUpdate(const FZeuzMatchMakingStatus& Status, FString Error)
//{
//	ZeuzMatchmakingStatus = Status;
//
//	if (Status.State == "retry")
//	{
//		StartMatchmaking(LastMatchmakeRegion);
//	}
//	if (!Error.IsEmpty())
//	{
//		MatchmakingStatus = EMatchmakingStatus::MS_Failure;
//		ZeuzMatchmakingStatus.Error = Error;
//		return;
//	}
//
//	if (!Status.Result.ServerConnect.IsEmpty())
//	{
//		MatchmakingStatus = EMatchmakingStatus::MS_Success;
//		// time to connect
//		ConnectionAttempt = 0;
//		TryConnectServer(Status.Result.ServerConnect);
//	}
//	else
//	{
//		GetWorld()->GetTimerManager().SetTimer(MatchMakingUpdate_Handle, this, &UZeuzMatchmakingWidget::MatchmakingUpdate, 5.0f, false);
//	}
//}
//
//void UZeuzMatchmakingWidget::TryConnectServer(FString ConnectIp)
//{
//	ConnectionAttempt++;
//	if (ConnectionAttempt < 5)
//	{
//		UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
//		gi->ConnectSteamServer(ConnectIp);
//		GetWorld()->GetTimerManager().SetTimer(TryConnectServer_Handle, FTimerDelegate::CreateUObject(this, &UZeuzMatchmakingWidget::TryConnectServer, ConnectIp), 10.0f, false);
//	}
//	else
//	{
//		MatchmakingStatus = EMatchmakingStatus::MS_Failure;
//		ZeuzMatchmakingStatus.Error = "Unable to connect to server.";
//	}
//
//}
//
//void UZeuzMatchmakingWidget::MatchmakingUpdate()
//{
//	UZeuzApiMatchmaking::MatchmakingUpdate(ZeuzMatchmakingStatus.MatchMakingId, MatchmakingUpdateDelegate);
//}
//
//void UZeuzMatchmakingWidget::StopMatchmaking()
//{
//	GetWorld()->GetTimerManager().ClearTimer(MatchMakingUpdate_Handle);
//	MatchmakingStatus = EMatchmakingStatus::MS_Cancelled;
//	
//	if (!ZeuzMatchmakingStatus.MatchMakingId.IsEmpty())
//	{
//		UZeuzApiMatchmaking::MatchmakingClose(ZeuzMatchmakingStatus.MatchMakingId);
//		ZeuzMatchmakingStatus = FZeuzMatchMakingStatus();
//	}
//	// AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
//	// if (pc)
//	// {
//	// 	pc->StopPartyMatchmaking(pc->PlayerState->GetPlayerName() + " cancelled the matchmaking");
//	// }
	// }
