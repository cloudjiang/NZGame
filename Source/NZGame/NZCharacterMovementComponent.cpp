// Fill out your copyright notice in the Description page of Project Settings.

#include "NZGame.h"
#include "NZCharacterMovementComponent.h"
#include "NZCharacter.h"
#include "GameFramework/GameNetworkManager.h"



const FName NAME_RunDist(TEXT("RunDist"));
const FName NAME_SprintDist(TEXT("SprintDist"));
const FName NAME_InAirDist(TEXT("InAirDist"));
const FName NAME_SwimDist(TEXT("SwimDist"));
                           


UNZCharacterMovementComponent::UNZCharacterMovementComponent()
{
	MaxWalkSpeedSprinting = 800;
}

void UNZCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	ANZCharacter* NZOwner = Cast<ANZCharacter>(CharacterOwner);
	UMovementComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	bool bOwnerIsRagdoll = NZOwner && NZOwner->IsRagdoll();
	if (bOwnerIsRagdoll)
	{
		// Ignore jump key presses this frame since the character is in ragdoll and they don't apply
		NZOwner->bPressedJump = false;
		if (!NZOwner->GetController())
		{
			return;
		}
	}

	const FVector InputVector = ConsumeInputVector();
	if (!HasValidData() || ShouldSkipUpdate(DeltaTime) || UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	if (CharacterOwner->Role > ROLE_SimulatedProxy)
	{
		if (CharacterOwner->Role == ROLE_Authority)
		{
			// Check we are still in the world, and stop simulating if not
			const bool bStillInWorld = (bCheatFlying || CharacterOwner->CheckStillInWorld());
			if (!bStillInWorld || !HasValidData())
			{
				return;
			}
		}

		// If we are a client we might have received an update from the server
		const bool bIsClient = (GetNetMode() == NM_Client && CharacterOwner->Role == ROLE_AutonomousProxy);
		if (bIsClient && !bOwnerIsRagdoll)
		{
			ClientUpdatePositionAfterServerUpdate();
		}

		// Allow root motion to move characters that have no controller
		if (CharacterOwner->IsLocallyControlled() || bRunPhysicsWithNoController || (!CharacterOwner->Controller && CharacterOwner->IsPlayingRootMotion()))
		{
			FNetworkPredictionData_Client_Character* ClientData = ((CharacterOwner->Role < ROLE_Authority) && (GetNetMode() == NM_Client)) ? GetPredictionData_Client_Character() : NULL;
			if (ClientData)
			{
				// Update our delta time for physics simulation
				DeltaTime = UpdateTimeStampAndDeltaTime(DeltaTime, ClientData);
				CurrentServerMoveTime = ClientData->CurrentTimeStamp;
			}
			else
			{
				CurrentServerMoveTime = GetWorld()->GetTimeSeconds();
			}

			// We need to check the jump state before adjusting input acceleration, 
			// to minimize latency and to make sure acceleration respects our potentially new falling state
			CharacterOwner->CheckJumpInput(DeltaTime);

			// Apply input to acceleration
			Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(InputVector));
			AnalogInputModifier = ComputeAnalogInputModifier();

			if ((CharacterOwner->Role == ROLE_Authority) && !bOwnerIsRagdoll)
			{
				PerformMovement(DeltaTime);
			}
			else if (bIsClient)
			{
				ReplicateMoveToServer(DeltaTime, Acceleration);
			}
		}
		else if ((CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy) && !bOwnerIsRagdoll)
		{
			// Server ticking for remote client
			// Between net updates from the client we need to update position if based on another object,
			// otherwise the object will move on intermediate frames and we won't follow it
			MaybeUpdateBasedMovement(DeltaTime);
			SaveBaseLocation();
		}
		else if (!CharacterOwner->Controller && (CharacterOwner->Role == ROLE_Authority) && !bOwnerIsRagdoll)
		{
			// Still update forces
			ApplyAccumulatedForces(DeltaTime);
			PerformMovement(DeltaTime);
		}
	}
	else if (!bOwnerIsRagdoll && CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		AdjustProxyCapsuleSize();
		SimulatedTick(DeltaTime);
		CharacterOwner->RecalculateBaseEyeHeight();
	}

	if (bEnablePhysicsInteraction && !bOwnerIsRagdoll)
	{
		if (CurrentFloor.HitResult.IsValidBlockingHit())
		{
			// Apply downwards force when walking on top of physics objects
			if (UPrimitiveComponent* BaseComp = CurrentFloor.HitResult.GetComponent())
			{
				if (StandingDownwardForceScale != 0.f && BaseComp->IsAnySimulatingPhysics())
				{
					const float GravZ = GetGravityZ();
					const FVector ForceLocation = CurrentFloor.HitResult.ImpactPoint;
					BaseComp->AddForceAtLocation(FVector(0.f, 0.f, GravZ * Mass * StandingDownwardForceScale), ForceLocation, CurrentFloor.HitResult.BoneName);
				}
			}
		}
	}

	if (bOwnerIsRagdoll)
	{
		// Ignore jump key presses this frame since the character is in ragdoll and they don't apply
		NZOwner->bPressedJump = false;
	}
	AvgSpeed = AvgSpeed * (1.f - 2.f * DeltaTime) + 2.f * DeltaTime * Velocity.Size2D();
	if (CharacterOwner != NULL)
	{
		ANZPlayerController* PC = Cast<ANZPlayerController>(CharacterOwner->Controller);
		if (PC != NULL && PC->PlayerInput != NULL)
		{
			PC->ApplyDeferredFireInputs();
		}
	}
}

float UNZCharacterMovementComponent::ComputeAnalogInputModifier() const
{
    return 1.f;
}

void UNZCharacterMovementComponent::UpdateMovementStats(const FVector& StartLocation)
{
    if (CharacterOwner && CharacterOwner->Role == ROLE_Authority)
    {
        ANZPlayerState* PS = Cast<ANZPlayerState>(CharacterOwner->PlayerState);
        if (PS)
        {
            float Dist = (GetActorLocation() - StartLocation).Size();
            FName MovementName = bIsSprinting ? NAME_SprintDist : NAME_RunDist;
            if (MovementMode == MOVE_Falling)
            {
                ANZCharacter* NZCharOwner = Cast<ANZCharacter>(CharacterOwner);
                MovementName = NAME_InAirDist;
            }
            else if (MovementMode == MOVE_Swimming)
            {
                MovementName = NAME_SwimDist;
            }
            PS->ModifyStatsValue(MovementName, Dist);
        }
    }
}

bool UNZCharacterMovementComponent::CanBaseOnLift(UPrimitiveComponent* LiftPrim, const FVector& LiftMoveDelta)
{
    // If character jumped off this lift and is still going up fast enough, then just push him along
    if (Velocity.Z > 0.f)
    {
        FVector LiftVel = MovementBaseUtility::GetMovementBaseVelocity(LiftPrim, NAME_None);
        if (LiftVel.Z > 0.f)
        {
            FHitResult Hit(1.f);
            FVector MoveDelta(0.f);
            MoveDelta.Z = LiftMoveDelta.Z;
            SafeMoveUpdatedComponent(MoveDelta, CharacterOwner->GetActorRotation(), true, Hit);
            return true;
        }
    }
    const FVector PawnLocation = CharacterOwner->GetActorLocation();
    FFindFloorResult FloorResult;
    FindFloor(PawnLocation, FloorResult, false);
    if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
    {
        if (IsFalling())
        {
            ProcessLanded(FloorResult.HitResult, 0.f, 1);
        }
        else if (IsMovingOnGround())
        {
            AdjustFloorHeight();
            SetBase(FloorResult.HitResult.Component.Get(), FloorResult.HitResult.BoneName);
        }
        else
        {
            return false;
        }
        return (CharacterOwner->GetMovementBase() == LiftPrim);
    }
    return false;
}

void UNZCharacterMovementComponent::UpdateBasedMovement(float DeltaSeconds)
{
    Super::UpdateBasedMovement(DeltaSeconds);
    
    // todo:
}

bool UNZCharacterMovementComponent::CheckFall(const FFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations, bool bMustJump)
{
    bool bResult = Super::CheckFall(OldFloor, Hit, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump);
    if (!bResult)
    {
        // todo:
    }
    return bResult;
}

void UNZCharacterMovementComponent::OnUnableToFollowBaseMove(const FVector& DeltaPosition, const FVector& OldLocation, const FHitResult& MoveOnBaseHit)
{
    UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
    
    if (CharacterOwner->Role == ROLE_SimulatedProxy)
    {
        // Force it since on client
        UpdatedComponent->SetWorldLocationAndRotation(UpdatedComponent->GetComponentLocation() + DeltaPosition, UpdatedComponent->GetComponentQuat(), false);
        return;
    }
    
    // todo:
    //if (MovementBase && Cast<ANZLift>(MovementBase->GetOwner()) && (MovementBase->GetOwner()->GetVelocity().Z >= 0.f))
    //{
    //    Cast<ANZLift>(MovementBase->GetOwner())->OnEncroachActor(CharacterOwner);
    //}
    
}

void UNZCharacterMovementComponent::ResetTimers()
{
    SprintStartTime = GetCurrentMovementTime() + AutoSprintDelayInterval;
}

bool UNZCharacterMovementComponent::Is3DMovementMode() const
{
    return (MovementMode == MOVE_Flying) || (MovementMode == MOVE_Swimming);
}

void UNZCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
    if (MovementMode == MOVE_Walking)
    {
        ANZCharacter* NZOwner = Cast<ANZCharacter>(CharacterOwner);
        if (NZOwner != NULL)
        {
            Friction *= (1.0f - NZOwner->GetWalkMovementReductionPct());
            BrakingDeceleration *= (1.0f - NZOwner->GetWalkMovementReductionPct());
        }
    }
    
    Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
    
    // Workaround for engine path following code not setting Acceleration correctly
    if (bHasRequestedVelocity && Acceleration.IsZero())
    {
        Acceleration = Velocity.GetSafeNormal();
    }
}

float UNZCharacterMovementComponent::UpdateTimeStampAndDeltaTime(float DeltaTime, FNetworkPredictionData_Client_Character* ClientData)
{
    float UnModifiedTimeStamp = ClientData->CurrentTimeStamp + DeltaTime;
    DeltaTime = ClientData->UpdateTimeStampAndDeltaTime(DeltaTime, *CharacterOwner, *this);
    if (ClientData->CurrentTimeStamp < UnModifiedTimeStamp)
    {
        // Client timestamp rolled over, so roll over our movement timers
        AdjustMovementTimers(UnModifiedTimeStamp - ClientData->CurrentTimeStamp);
    }
    return DeltaTime;
}

void UNZCharacterMovementComponent::AdjustMovementTimers(float Adjustment)
{
    SprintStartTime -= Adjustment;
}

bool UNZCharacterMovementComponent::NZVerifyClientTimeStamp(float TimeStamp, FNetworkPredictionData_Server_Character& ServerData)
{
    // Very large deltas happen around a TimeStamp reset.
    const float DeltaTimeStamp = (TimeStamp - ServerData.CurrentClientTimeStamp);
    if (FMath::Abs(DeltaTimeStamp) > (MinTimeBetweenTimeStampResets * 0.5f))
    {
        // Client is resetting TimeStamp to increase accuracy.
        if (DeltaTimeStamp < 0.f)
        {
            ServerData.CurrentClientTimeStamp = 0.f;
            AdjustMovementTimers(-1.f * DeltaTimeStamp);
            return true;
        }
        else
        {
            // We already reset the TimeStamp, but we just got an old outdated move before the switch.
            // Just ignore it.
            return false;
        }
    }
    
    // If TimeStamp is in the past, move is oudated, ignore it.
    if (TimeStamp <= ServerData.CurrentClientTimeStamp)
    {
        return false;
    }
    
    // SpeedHack detection: warn if the timestamp error limit is exceeded, and clamp
    ANZGameMode* NZGameMode = GetWorld()->GetAuthGameMode<ANZGameMode>();
    if (NZGameMode != NULL)
    {
        float ServerDelta = GetWorld()->GetTimeSeconds() - ServerData.ServerTimeStamp;
        float ClientDelta = FMath::Min(TimeStamp - ServerData.CurrentClientTimeStamp, AGameNetworkManager::StaticClass()->GetDefaultObject<AGameNetworkManager>()->MAXCLIENTUPDATEINTERVAL);
        float CurrentError = (ServerData.CurrentClientTimeStamp != 0.f) ? ClientDelta - ServerDelta * (1.f + NZGameMode->TimeMarginSlack) : 0.f;
        TotalTimeStampError = FMath::Max(TotalTimeStampError + CurrentError, NZGameMode->MinTimeMargin);
        bClearingSpeedHack = bClearingSpeedHack && (TotalTimeStampError > 0.f);
        if (bClearingSpeedHack)
        {
            TotalTimeStampError -= ClientDelta;
        }
        else if (TotalTimeStampError > NZGameMode->MaxTimeMargin)
        {
            TotalTimeStampError -= ClientDelta;
            bClearingSpeedHack = true;
            NZGameMode->NotifySpeedHack(CharacterOwner);
        }
    }
    
    return true;
}

void UNZCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
    
}




float UNZCharacterMovementComponent::GetMaxSpeed() const
{
	ANZCharacter* NZCharacter = Cast<ANZCharacter>(CharacterOwner);
	if (NZCharacter != NULL && NZCharacter->bSprinting)
	{
		return MaxWalkSpeedSprinting;
	}
	else
	{
		return Super::GetMaxSpeed();
	}
}

float UNZCharacterMovementComponent::GetCurrentMovementTime() const
{
    return 0.f;
}

float UNZCharacterMovementComponent::GetCurrentSynchTime() const
{
    return 0.f;
}
