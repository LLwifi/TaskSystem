// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <TS_StructAndEnum.h>
#include "TS_FunctionLibrary.generated.h"

/**
 * 任务系统的通用函数库
 */
UCLASS()
class TASKSYSTEM_API UTS_FunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	//任务目标是否完成 
	UFUNCTION(BlueprintPure)
	static bool TaskTargetIsComplete(FTaskTargetInfo TaskTargetInfo);
};
