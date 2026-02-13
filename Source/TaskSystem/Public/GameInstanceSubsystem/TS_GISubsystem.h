// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include <TS_StructAndEnum.h>
#include "TS_GISubsystem.generated.h"

class UTS_Task;

USTRUCT(BlueprintType)
struct FSameIDTask
{
	GENERATED_BODY()
public:
	
	FSameIDTask() {};
	FSameIDTask(UTS_Task* Task) { Add(Task); };
	UTS_Task* operator[](int32 Index) { return SameIDTask[Index]; };
	int32 Num() { return SameIDTask.Num(); };
	void Add(UTS_Task* Task) { SameIDTask.Add(Task); };
	void Remove(UTS_Task* Task){ SameIDTask.Remove(Task); }
	bool IsValidIndex(int32 Index){ return SameIDTask.IsValidIndex(Index); }
	UTS_Task* Last(){ return SameIDTask.Last(); }

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UTS_Task*> SameIDTask;
};

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
	//通过配置ID创建任务
	UFUNCTION(BlueprintCallable)
	UTS_Task* CreateTaskFromID(int32 TaskID);

	//通过唯一ID获取任务
	UFUNCTION(BlueprintCallable)
	UTS_Task* GetTaskFromUniqueID(int32 UniqueID);

	//通过配置ID获取任务
	UFUNCTION(BlueprintCallable)
	FSameIDTask GetTaskFromID(int32 ID);

public:
	//全部任务<唯一任务ID，任务> 该值仅在服务器有效
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, UTS_Task*> AllTask_UniqueID;

	//全部任务<配置任务ID，任务> 该值仅在服务器有效
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, FSameIDTask> AllTask_ID;

	//唯一IDMap<任务ID，第几个> 该值仅在服务器有效
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, int32> AllUniqueID;
};
