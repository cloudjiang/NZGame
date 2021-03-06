// Fill out your copyright notice in the Description page of Project Settings.

#include "NZGame.h"
#include "NZPlayerState.h"
#include "NZPlayerController.h"
#include "NZGameMessage.h"



void ANZPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void ANZPlayerState::SetCharacter(const FString& CharacterPath)
{
    // todo:
	check(false);
}

void ANZPlayerState::ServerSetCharacter_Implementation(const FString& CharacterPath)
{
	// todo:
	check(false);
}

bool ANZPlayerState::ServerSetCharacter_Validate(const FString& CharacterPath)
{
	return true;
}

void ANZPlayerState::UpdatePing(float InPing)
{

}

void ANZPlayerState::CalculatePing(float NewPing)
{
	if (NewPing < 0.f)
	{
		// Caused by timestamp wrap around
		return;
	}

	float OldPing = ExactPing;
	Super::UpdatePing(NewPing);

	ANZPlayerController* PC = Cast<ANZPlayerController>(GetOwner());
	if (PC)
	{
		PC->LastPingCalcTime = GetWorld()->GetTimeSeconds();
		if (ExactPing != OldPing)
		{
			PC->ServerUpdatePing(ExactPing);
		}
	}
}

void ANZPlayerState::HandleTeamChanged(AController* Controller)
{
    ANZCharacter* Pawn = Cast<ANZCharacter>(Controller->GetPawn());
    if (Pawn != NULL)
    {
        Pawn->PlayerChangedTeam();
    }
    if (Team)
    {
        int32 Switch = (Team->TeamIndex == 0) ? 9 : 10;
        ANZPlayerController* PC = Cast<ANZPlayerController>(Controller);
        if (PC)
        {
            PC->ClientReceiveLocalizedMessage(UNZGameMessage::StaticClass(), Switch, this, NULL, NULL);
        }
    }
}

void ANZPlayerState::NotifyTeamChanged_Implementation()
{
    for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
    {
        ANZCharacter* P = Cast<ANZCharacter>(*It);
        if (P != NULL && P->PlayerState == this && !P->bTearOff)
        {
            P->NotifyTeamChanged();
        }
    }
    // HACK: Remember last team player got on the URL for travelling purposes
    if (Team != NULL)
    {
        ANZPlayerController* PC = Cast<ANZPlayerController>(GetOwner());
        if (PC != NULL)
        {
            UNZLocalPlayer* LP = Cast<UNZLocalPlayer>(PC->Player);
            if (LP != NULL)
            {
                LP->SetDefaultURLOption(TEXT("Team"), FString::FromInt(Team->TeamIndex));
            }
        }
    }
}

void ANZPlayerState::IncrementKills(TSubclassOf<UDamageType> DamageType, bool bEnemyKill, ANZPlayerState* VictimPS)
{
    
}

void ANZPlayerState::IncrementDeaths(TSubclassOf<UDamageType> DamageType, ANZPlayerState* KillerPlayerState)
{
    
}

void ANZPlayerState::AdjustScore(int32 ScoreAdjustment)
{
	Score += ScoreAdjustment;
	ForceNetUpdate();
}

void ANZPlayerState::OnRep_Deaths()
{
	// todo:
	check(false);
}

uint8 ANZPlayerState::GetTeamNum() const
{
	return (Team != NULL) ? Team->GetTeamNum() : 255;
}


float ANZPlayerState::GetStatsValue(FName StatsName) const
{
    return StatsData.FindRef(StatsName);
}

void ANZPlayerState::SetStatsValue(FName StatsName, float NewValue)
{
    LastScoreStatsUpdateTime = GetWorld()->GetTimeSeconds();
    StatsData.Add(StatsName, NewValue);
}

void ANZPlayerState::ModifyStatsValue(FName StatsName, float Change)
{
    LastScoreStatsUpdateTime = GetWorld()->GetTimeSeconds();
    float CurrentValue = StatsData.FindRef(StatsName);
    StatsData.Add(StatsName, CurrentValue + Change);
}


float ANZPlayerState::GetAvailableCurrency()
{
    return AvailableCurrency;
}

void ANZPlayerState::AdjustCurrency(float Adjustment)
{
    AvailableCurrency += Adjustment;
    if (AvailableCurrency < 0.0)
    {
        AvailableCurrency = 0.0f;
    }
}

