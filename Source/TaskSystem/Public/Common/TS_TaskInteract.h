#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include <ActorComponent/TS_TaskComponent.h>
#include "TS_TaskInteract.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTS_TaskInteract : public UInterface
{
	GENERATED_BODY()
};

/**
 * 任务相关接口
 */
class TASKSYSTEM_API ITS_TaskInteract
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	///*
	//* 刷新任务目标（触发刷新的任务组件）
	//*/
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	//	void RefreshTaskTarget(UTS_TaskComponent* TriggerTaskComponent);
	//virtual void RefreshTaskTarget_Implementation(UTS_TaskComponent* TriggerTaskComponent){};
};
