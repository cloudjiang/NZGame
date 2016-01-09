// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NZInventory.h"
#include "NZWeapon.generated.h"

/**
 * 
 */
UCLASS(Config = Game)
class NZGAME_API ANZWeapon : public ANZInventory
{
	GENERATED_BODY()
    
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, ReplicatedUsing = OnRep_AttachmentType, Category = Weapon)
    TSubclassOf<class ANZWeaponAttachment> AttachmentType;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, ReplicatedUsing = OnRep_Ammo, Category = Weapon)
    int32 Ammo;
    
    UFUNCTION()
    virtual void OnRep_AttachmentType();
    
    UFUNCTION()
    virtual void OnRep_Ammo();
    
    
    
    UFUNCTION(BlueprintCallable, Category = Weapon)
    virtual bool HasAnyAmmo();


    
	virtual void BringUp(float OverflowTime = 0.0f);

	virtual bool PutDown();
	
    
    UPROPERTY(EditAnywhere, Config, Transient, BlueprintReadOnly, Category = Selection)
    float AutoSwitchPriority;
    
    
};
