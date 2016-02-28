// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NZWeapon.h"
#include "NZRealGun.generated.h"

/**
 * 
 */
UCLASS()
class NZGAME_API ANZRealGun : public ANZWeapon
{
	GENERATED_BODY()
	
public:
	int ShotsFired;

	int GunTurnRedStartShots;

	bool bIsNoTarget;

	float StartNoTargetTime;

	bool bDelayOneFrameForCamera;

public:
    UPROPERTY()
    int FireCount;
    
    UPROPERTY()
    int CDFireCount;
    
    UPROPERTY()
    float DeltaYawParam;
    
    UPROPERTY()
    float DeltaPitchParam;
    
    UPROPERTY()
    float RecordDeltaYawParam;
    
    UPROPERTY()
    float RecordDeltaPitchParam;
    
    UPROPERTY()
    float CurrentFireTime;
    
    UPROPERTY()
    float CurrentStopFireTime;
    
    UPROPERTY()
    float ReactParam;
    
    UPROPERTY()
    float RecordReactParam;
    
    //
    UPROPERTY()
    TArray<float> ReactParamCoefficient;
    
    UPROPERTY()
    TArray<float> FireCounterInterval;
    
    UPROPERTY()
    TArray<float> ChangeMovingRealSize;
    
    UPROPERTY()
    TArray<float> DetailReactYawShot;
    
    UPROPERTY()
    TArray<float> DetailReactPitchShot;
    
    UPROPERTY()
    TArray<float> FullReactYawCoefficient;
    
    UPROPERTY()
    TArray<float> FullReactPitchCoefficient;
    
    UPROPERTY()
    TArray<float> PertunrbMax;
    
    UPROPERTY()
    TArray<float> PertunrbMin;
    
    UPROPERTY()
    TArray<float> CrosshairRatioPerRealSize;
    
    //
    UPROPERTY()
    int SideReactDirectSectionNum;
    
    UPROPERTY()
    int SideReactDirectOneSectionNum;
    
    UPROPERTY()
    TArray<int> SideReactDirectSection;
    
    UPROPERTY()
    TArray<float> SideReactDirect;
    
    //
    UPROPERTY()
    int ShotReactYawSectionNum;
    
    UPROPERTY()
    int ShotReactYawOneSectionNum;
    
    UPROPERTY()
    TArray<int> ShotReactYawSection;
    
	UPROPERTY()
    TArray<float> ShotReactYaw;
    
    //
    UPROPERTY()
    int ShotReactPitchSectionNum;
    
    UPROPERTY()
    int ShotReactPitchOneSectionNum;
    
    UPROPERTY()
    TArray<int> ShotReactPitchSection;
    
    UPROPERTY()
    TArray<float> ShotReactPitch;
    
    //
    UPROPERTY()
    int CameraYawAndPitchSectionNum;
    
    UPROPERTY()
    int CameraYawAndPitchOneSectionNum;
    
    UPROPERTY()
    TArray<int> CameraYawAndPitchSection;
    
    UPROPERTY()
    TArray<float> CameraYawAndPitch;
	
    
    UPROPERTY()
    bool bReachYawMax;
    
    UPROPERTY()
    int DelayFrameCount;
    
    UPROPERTY()
    int FireCountDelayFrameCount;
    
    UPROPERTY()
    float RecordEndFireReactParam;
    
    UPROPERTY()
    FQuat FireRotationQuat;
    
    UPROPERTY()
    int AdjustDeltaYawDirection;
    
    UPROPERTY()
    int ChangeDirectionFlag3;
    
    UPROPERTY()
    int ShotSpreadFlag;
    
    UPROPERTY()
    bool bChangeDirectionFlag1;
    
    UPROPERTY()
    float DamageFactorByDistance;
    
    UPROPERTY()
    float DamageVariantionFactor;
    

	virtual void SetPunchAngle(FRotator Angle);
	virtual FRotator GetPunchAngle();

	virtual void KickBackTheView();
    
	virtual void WeaponCalcCamera(float DeltaTime, FVector& OutCamLoc, FRotator& OutCamRot) override;

    
};