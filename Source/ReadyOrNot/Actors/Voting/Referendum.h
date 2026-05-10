// Copyright Void Interactive, 2017

#pragma once
#include "GameFramework/Actor.h"
#include "Referendum.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AReferendum : public AActor
{
	GENERATED_BODY()

public:
	AReferendum();

	// The name of the referendum, e.g, "Kick Vote"
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Referendum)
		FText ReferendumName;

	// A description of the referendum
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Referendum)
		FText ReferendumDescription;

	// How much time players have to vote on this referendum
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Referendum)
		float ReferendumTime = 30.0f;

	// How much time between the announcement of the vote passing and the action taking effect
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Referendum)
		float ReferendumHoldingTime = 3.0f;

	// What happens when the referendum passes
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Referendum)
		void OnReferendumPassed();
	virtual void OnReferendumPassed_Implementation() {}

	// How many yes votes there are for the referendum currently
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Referendum)
		int32 YesVotes;

	// How many no votes there are for the referendum currently
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Referendum)
		int32 NoVotes;

	// How much time is remaining in the referendum
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Referendum)
		float ReferendumTimeRemaining = 0.0f;

	// How much time is remaining to wait before executing the passing vote action
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Referendum)
		float ReferendumHoldingTimeRemaining = 3.0f;

	// Whether the referendum is running
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Referendum)
		bool bReferendumRunning = false;

	// Whether the referendum is waiting to take effect
	UPROPERTY(BlueprintReadOnly, Category = Referendum)
		bool bReferendumWaitingToTakeEffect = false;

	// Who called the referendum
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Referendum)
		class AReadyOrNotPlayerState* ReferendumCaller;

	// Who has voted yes in the current referendum
	UPROPERTY(BlueprintReadOnly, Category = Referendum)
		TArray<class AReadyOrNotPlayerController*> YesVoters;

	// Who has voted no in the current referendum
	UPROPERTY(BlueprintReadOnly, Category = Referendum)
		TArray<class AReadyOrNotPlayerController*> NoVoters;

	// Who is eligible to vote in the current referendum
	UPROPERTY(BlueprintReadOnly, Category = Referendum)
		TArray<class AReadyOrNotPlayerController*> EligibleVoters;

	// A yes vote has been cast from someone
	UFUNCTION(BlueprintCallable)
		void CastedYesVote(class AReadyOrNotPlayerController* Voter);

	UFUNCTION(BlueprintCallable)
		void CastedNoVote(class AReadyOrNotPlayerController* Voter);

	// Multicast event - announce that the vote has started
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
		void Multicast_AnnounceVoteStarted(class AReadyOrNotPlayerState* CallingVoter);
	virtual void Multicast_AnnounceVoteStarted_Implementation(class AReadyOrNotPlayerState* CallingVoter);

	// Multicast event - announce that the vote passed
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
		void Multicast_AnnounceVotePassed();
	virtual void Multicast_AnnounceVotePassed_Implementation();

	// Multicast event - announce that the vote failed
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
		void Multicast_AnnounceVoteFailed();
	virtual void Multicast_AnnounceVoteFailed_Implementation();

	// Multicast event - announce that someone has voted yes
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
		void Multicast_AnnounceYesVote(class AReadyOrNotPlayerState* Voter);
	virtual void Multicast_AnnounceYesVote_Implementation(class AReadyOrNotPlayerState* Voter);

	// Multicast event - announce that someone has voted no
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
		void Multicast_AnnounceNoVote(class AReadyOrNotPlayerState* Voter);
	virtual void Multicast_AnnounceNoVote_Implementation(class AReadyOrNotPlayerState* Voter);

	// Blueprint event: announce that a vote has started
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Referendum)
		void OnAnnounceVoteStarted(class AReadyOrNotPlayerState* CallingVoter);

	// Blueprint event: announce that a vote passed
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Referendum)
		void OnAnnounceVotePassed();

	// Blueprint event: announce that a vote failed
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Referendum)
		void OnAnnounceVoteFailed();

	// Blueprint event: announce that someone has voted yes
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Referendum)
		void OnAnnounceYesVote(class AReadyOrNotPlayerState* Voter);

	// Blueprint event: announce that someone has voted no
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Referendum)
		void OnAnnounceNoVote(class AReadyOrNotPlayerState* Voter);

	// Get the text that appears on the HUD
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure, Category = Referendum)
		FText GetHudDescription();
	virtual FText GetHudDescription_Implementation() { return ReferendumName; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
};