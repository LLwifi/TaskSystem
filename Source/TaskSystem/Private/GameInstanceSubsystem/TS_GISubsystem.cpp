// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstanceSubsystem/TS_GISubsystem.h"
#include "TS_Task.h"
#include <Common/TS_TaskConfig.h>

UTS_Task* UTS_GISubsystem::CreateTaskFromInfo(FTaskInfo TaskInfo)
{
	UClass* TaskClass = TaskInfo.TaskClass.LoadSynchronous();
	if (TaskClass)
	{
		UTS_Task* Task = NewObject<UTS_Task>(this, TaskClass);
		if (Task)
		{
			Task->TaskInfo = TaskInfo;
			//生成唯一ID 任务ID长度 + 任务ID + 第几个同类任务
			FString UniqueIDStr = FString::FromInt(FString::FromInt(TaskInfo.TaskID).Len()) + FString::FromInt(TaskInfo.TaskID);
			if (UniqueID.Contains(TaskInfo.TaskID))
			{
				UniqueID[TaskInfo.TaskID]++;
			}
			else
			{
				UniqueID.Add(TaskInfo.TaskID,1);
			}
			UniqueIDStr += FString::FromInt(UniqueID[TaskInfo.TaskID]);
			Task->TaskUniqueID = FCString::Atoi(*UniqueIDStr);
			return Task;
		}
	}
	return nullptr;
}

UTS_Task* UTS_GISubsystem::CreateTaskFromID(int32 TaskID)
{
	UDataTable* DT = UTS_TaskConfig::GetInstance()->AllTask.LoadSynchronous();
	if (DT)
	{
		FTaskInfo* TaskInfo = DT->FindRow<FTaskInfo>(FName(*FString::FromInt(TaskID)), TEXT(""));
		if (TaskInfo)
		{
			return CreateTaskFromInfo(*TaskInfo);
		}
	}
	return nullptr;
}
