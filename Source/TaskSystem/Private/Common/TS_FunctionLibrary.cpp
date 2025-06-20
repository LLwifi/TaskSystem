// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/TS_FunctionLibrary.h"

bool UTS_FunctionLibrary::TaskTargetIsComplete(FTaskTargetInfo TaskTargetInfo)
{
    return TaskTargetInfo.TaskTargetIsComplete();
}
