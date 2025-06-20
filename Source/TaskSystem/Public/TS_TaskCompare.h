// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <TS_StructAndEnum.h>
#include "TS_TaskCompare.generated.h"


/**
 * 任务对照/比较基类类
 * 任务系统中有许多条件需要进行比较，过于定制化的对比需求使用该类进行实现
 */
UCLASS(Blueprintable, BlueprintType)
class TASKSYSTEM_API UTS_TaskCompare : public UObject
{
	GENERATED_BODY()
	
public:
	//比较结果
	UFUNCTION(BlueprintNativeEvent)
		bool CompareResult(FTaskCompareInfo ThisCompareInfo, FRefreshTaskTargetInfo RefreshTaskTargetInfo);
	virtual bool CompareResult_Implementation(FTaskCompareInfo ThisCompareInfo, FRefreshTaskTargetInfo RefreshTaskTargetInfo);
};
