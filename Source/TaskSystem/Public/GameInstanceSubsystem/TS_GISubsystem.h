// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include <TS_StructAndEnum.h>
#include "TS_GISubsystem.generated.h"

class UTS_Task;

/**
 * 任务系统的GI
 * 主要创建任务，且为每个任务分配一个唯一ID
 * 唯一ID：用于任务判断，即使两个完全一致的任务，唯一ID也不同
 */
UCLASS()
class TASKSYSTEM_API UTS_GISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	//通过信息创建任务
	UFUNCTION(BlueprintCallable)
	UTS_Task* CreateTaskFromInfo(FTaskInfo TaskInfo);
	//通过ID创建任务
	UFUNCTION(BlueprintCallable)
	UTS_Task* CreateTaskFromID(int32 TaskID);

public:
	//全部任务
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UTS_Task*> AllTask;

	//唯一IDMap<任务ID，第几个>
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, int32> UniqueID;
};
