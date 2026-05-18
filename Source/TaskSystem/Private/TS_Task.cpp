// Fill out your copyright notice in the Description page of Project Settings.


#include "TS_Task.h"
#include <Kismet/KismetSystemLibrary.h>
#include "TS_TaskCompare.h"
#include <Common/TS_TaskConfig.h>
#include "ActorComponent/TS_TaskComponent.h"
#include <Kismet/GameplayStatics.h>
#include <GameInstanceSubsystem/TS_GISubsystem.h>

UTS_Task::UTS_Task()
{
	if (AllTaskComponent.Num() == 0)
	{
		UTS_TaskComponent* TaskComponent = Cast<UTS_TaskComponent>(GetOuter());
		if (TaskComponent)
		{
			AllTaskComponent.Add(TaskComponent);
		}
		if (AllTaskComponent.Num() == 0)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Task Outer Must Be UTS_TaskComponent Then Can RPC"));
		}
	}

	//如果该值在构造时已经为true，说明服务器添加任务尝试同步属性时，客户端的任务还未创建。 为了保证客户端的流程正确（UI显示等），这里额外调用开始事件
	if (bTaskIsStart)
	{
		StartTask();
	}
}

UWorld* UTS_Task::GetWorld() const
{
	UObject* outer = GetOuter();
	if (outer && (outer->IsA<AActor>() || outer->IsA<UActorComponent>() || outer->IsA<USubsystem>()) && !outer->HasAnyFlags(RF_ClassDefaultObject))
	{
		return outer->GetWorld();
	}
	return nullptr;
}

bool UTS_Task::IsSupportedForNetworking() const
{
	return true;
}

void UTS_Task::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTS_Task, TaskInfo);
	DOREPLIFETIME(UTS_Task, TaskTime);

	DOREPLIFETIME(UTS_Task, TaskUniqueID);
	DOREPLIFETIME(UTS_Task, bTaskIsComplete);
	DOREPLIFETIME(UTS_Task, bTaskIsStart);
	DOREPLIFETIME(UTS_Task, bTaskIsFinish);
	DOREPLIFETIME(UTS_Task, bTaskIsEnd);
	DOREPLIFETIME(UTS_Task, RoleSigns);

	DOREPLIFETIME(UTS_Task, TaskMarkActors);
	DOREPLIFETIME(UTS_Task, TaskMarkLocation);
	DOREPLIFETIME(UTS_Task, CurUpdateTaskTarget);
}

void UTS_Task::ReplicatedUsing_TaskInfoChange()
{
	TaskUpdate();
}

void UTS_Task::ReplicatedUsing_TaskTime()
{
	TaskTimeUpdate();
}

void UTS_Task::ReplicatedUsing_CurUpdateTaskTarget()
{
	//客户端的相关函数在同步后调用
	TArray<FTaskTargetInfo> UpdateTaskTargetArray = CurUpdateTaskTarget;
	for (FTaskTargetInfo& TaskTargetInfo : UpdateTaskTargetArray)
	{
		if (!TaskTargetInfo.TaskTargetText.IsEmpty())//标题为空的是无效目标
		{
			TaskTargetUpdate(TaskTargetInfo);
		}
	}
	CurUpdateTaskTarget.Empty();
}

void UTS_Task::ReplicatedUsing_bTaskIsInitChange()
{
	InitTask();
}

void UTS_Task::ReplicatedUsing_bTaskIsStartChange()
{
	StartTask();
}

void UTS_Task::ReplicatedUsing_bTaskIsFinishChange()
{
	TaskFinish();
}

void UTS_Task::ReplicatedUsing_bTaskIsEndChange()
{
	TaskEnd();
}

void UTS_Task::ReplicatedUsing_TaskMarkLocation()
{
	if (/*TaskInfo.*/bIsMarkTask)
	{
		MarkTaskShow();
	}
}

void UTS_Task::ReplicatedUsing_TaskMarkActors()
{
	if (/*TaskInfo.*/bIsMarkTask)
	{
		MarkTaskShow();
	}
}

void UTS_Task::ServerRefreshOneTaskTargetFromID(FRefreshTaskTargetInfo RefreshTaskTargetInfo, int32 TaskTargetID)
{
	FTaskTargetInfo* TaskTarget = nullptr;
	if (GetTaskTargetFromID(TaskTargetID, TaskTarget))
	{
		ServerRefreshOneTaskTargetFromInfo(RefreshTaskTargetInfo, *TaskTarget);
	}
}

void UTS_Task::ServerRefreshOneTaskTargetFromInfo(FRefreshTaskTargetInfo RefreshTaskTargetInfo, UPARAM(Ref)FTaskTargetInfo& TaskTargetInfo)
{
	if (!TaskTargetInfo.bTaskTargetIsEnd && (RefreshTaskTargetInfo.RefreshTaskTargetID == TaskTargetInfo.TaskTargetID ||//ID相同直接通过
		(TaskTargetInfo.bCompareInfoIsOverride && SomeTaskTargetUpdateCheck(TaskTargetInfo, RefreshTaskTargetInfo))))//开启除了ID之外的其他检测方式 && 经过自定义的二次比对后可以增加进度
	{
		FTaskTargetInfo TempTaskTargetInfo = TaskTargetInfo;
		//通过比对
		if (TaskTargetInfo.AddProgress(RefreshTaskTargetInfo.AddProgress, RefreshTaskTargetInfo.RoleSign))//触发客户端的TaskUpdate同步函数
		{
			TempTaskTargetInfo = TaskTargetInfo;
			SomeTaskTargetEnd(TaskTargetInfo, true, RefreshTaskTargetInfo.RoleSign);
		}
		TaskTargetUpdate(TempTaskTargetInfo);//服务器调用该函数
		TaskTargetRoleSign.Add(TempTaskTargetInfo.TaskTargetID, RefreshTaskTargetInfo.RoleSign);//记录最新的签名
	}
}

void UTS_Task::ServerRefreshTaskTargetFromInfo(FRefreshTaskTargetInfo RefreshTaskTargetInfo)
{
	if (bTaskIsEnd)//已经结束的任务不应该再判断了
	{
		return;
	}
	TArray<FTaskTargetInfo> AllTaskTargetInfo = TaskInfo.TaskTargetInfo;//某个任务完成后可能会增加目标，为了防止在遍历中发生改变，这里复制一份数据出来进行遍历
	int32 CompleteTaskTargetNum = 0;//完成的普通任务目标数量
	int32 CompleteTaskTargetNum_MustDo = 0;//完成的必做任务目标数量
	int32 TaskTargetEndNum = 0;//结束的任务目标数量
	for (int32 i = 0; i < AllTaskTargetInfo.Num(); i++)
	{
		ServerRefreshOneTaskTargetFromInfo(RefreshTaskTargetInfo, TaskInfo.TaskTargetInfo[i]);
		if (TaskInfo.TaskTargetInfo[i].bTaskTargetIsEnd)//目标已经结束
		{
			TaskTargetEndNum++;
			if (TaskInfo.TaskTargetInfo[i].TaskTargetIsComplete())
			{
				if (TaskInfo.TaskTargetInfo[i].TaskTargetIsMustDo())//目标是否是必做
				{
					CompleteTaskTargetNum_MustDo++;
				}
				else
				{
					CompleteTaskTargetNum++;
				}
			}
		}
	}
	TaskEndCheckOfParameter(CompleteTaskTargetNum, CompleteTaskTargetNum_MustDo, TaskTargetEndNum);
}

void UTS_Task::ServerRefreshTaskTargetFromInfoArray(const TArray<FRefreshTaskTargetInfo>& RefreshTaskTargetInfoArray)
{
	for (int32 i = 0; i < RefreshTaskTargetInfoArray.Num(); i++)
	{
		ServerRefreshTaskTargetFromInfo(RefreshTaskTargetInfoArray[i]);
	}
}

void UTS_Task::ServerRefreshTaskTargetFromComponent(UTS_TaskComponent* TriggerTaskComponent, FName RoleSign)
{
	if (TriggerTaskComponent)
	{
		for (FRefreshTaskTargetInfo RefreshInfo : TriggerTaskComponent->RefreshTaskTargetInfos)
		{
			RefreshInfo.RoleSign = RoleSign;
			ServerRefreshTaskTargetFromInfo(RefreshInfo);
		}
	}
}

bool UTS_Task::TryInitTaskInfoFromDataTable(FTaskInfo& DataTaskInfo, FTaskInfo& DTTaskInfo)
{
	bool ReturnBool = false;
	UDataTable* DT = UTS_TaskConfig::GetInstance()->AllTask.LoadSynchronous();
	if (DT)
	{
		FTaskInfo* TaskInfo_DT = DT->FindRow<FTaskInfo>(FName(*FString::FromInt(DataTaskInfo.TaskID)), TEXT(""));
		if (TaskInfo_DT)
		{
			DTTaskInfo = *TaskInfo_DT;
			DataTaskInfo = ModifyTaskInfo(DTTaskInfo);
			ReturnBool = true;
		}
		else//如果找不到该任务认为是一个自定义的任务
		{
		}
	}

	//任务目标初始化
	FTaskTargetInfo TaskTargetInfoTemp;
	for (FTaskTargetInfo& TargetInfo : DataTaskInfo.TaskTargetInfo)
	{
		if (!TargetInfo.bIsCustom)//不是自定义的任务目标才需要尝试去读表初始化
		{
			TryInitTaskTargetFromDataTable(TargetInfo, TaskTargetInfoTemp);
		}
	}

	return ReturnBool;
}

FTaskInfo UTS_Task::ModifyTaskInfo_Implementation(FTaskInfo DataTaskInfo)
{
	return DataTaskInfo;
}

bool UTS_Task::TryInitTaskTargetFromDataTable(FTaskTargetInfo& DataTaskTarget, FTaskTargetInfo& DTTaskTargetInfo)
{
	UDataTable* DT = UTS_TaskConfig::GetInstance()->AllTaskTarget.LoadSynchronous();
	if (DT)
	{
		FTaskTargetInfo* TaskTargetInfo_DT = DT->FindRow<FTaskTargetInfo>(FName(*FString::FromInt(DataTaskTarget.TaskTargetID)), TEXT(""));
		if (TaskTargetInfo_DT)
		{
			DTTaskTargetInfo = *TaskTargetInfo_DT;
			DataTaskTarget.InitFromTaskTargetInfo(DTTaskTargetInfo);//通过其他任务目标信息初始化自身
			DataTaskTarget = ModifyTaskTargetInfo(DataTaskTarget);
			return true;
		}
	}

	return false;
}

FTaskTargetInfo UTS_Task::ModifyTaskTargetInfo_Implementation(FTaskTargetInfo DataTaskTarget)
{
	return DataTaskTarget;
}

void UTS_Task::InitTask_Implementation()
{
	bTaskIsInit = true;
	if (UKismetSystemLibrary::IsServer(this))//该函数首次由UTS_TaskComponent在服务器调用
	{
		FTaskInfo TaskInfoTemp;
		TryInitTaskInfoFromDataTable(TaskInfo, TaskInfoTemp);//查表初始化
	}
}

void UTS_Task::StartTask_Implementation()
{
	if (!bTaskIsStart)//避免重复调用开始
	{
		if (UKismetSystemLibrary::IsServer(this))//该函数首次由UTS_TaskComponent在服务器调用
		{
			//计时器只在服务器上开启
			if (TaskInfo.TaskTime > 0.0f)
			{
				ServerReSetTaskTime(TaskInfo.TaskTime);
			}

			TArray<FTaskTargetInfo> InitTaskTarget = TaskInfo.TaskTargetInfo;
			TaskInfo.TaskTargetInfo.Empty();//清空，通过下面for循环处理完后再重新添加
			for (FTaskTargetInfo& TaskTargetInfo : InitTaskTarget)
			{
				ServerAddTaskTarget(TaskTargetInfo);
			}

			bTaskIsStart = true;//触发客户端的StartTask同步函数
		}
		TaskStartEvent.Broadcast(this);
	}
}

void UTS_Task::TaskUpdate_Implementation()
{
	TaskUpdateEvent.Broadcast(this);
}

void UTS_Task::TaskTimeUpdate_Implementation()
{

}

void UTS_Task::TaskTargetUpdate_Implementation(FTaskTargetInfo UpdateTaskTarget)
{
	CurUpdateTaskTarget.Add(UpdateTaskTarget);
	TaskUpdate();

	//服务器的相关函数通常会立刻调用
	if (UpdateTaskTarget.bTaskTargetIsEnd)
	{
		NetMultiTaskTargetEnd(UpdateTaskTarget, UpdateTaskTarget.TaskTargetIsComplete());
	}
	else if (TaskInfo.TaskTargetInfo.Num() > LastAllTaskTargetInfo.Num())
	{
		for (int32 i = LastAllTaskTargetInfo.Num(); i < TaskInfo.TaskTargetInfo.Num(); i++)
		{
			NetMultiAddNewTaskTarget(TaskInfo.TaskTargetInfo[i]);
		}
		LastAllTaskTargetInfo = TaskInfo.TaskTargetInfo;
	}
}

void UTS_Task::NetMultiAddNewTaskTarget_Implementation(FTaskTargetInfo NewTaskTarget)
{

}

void UTS_Task::NetMultiTaskTargetEnd_Implementation(FTaskTargetInfo EndTaskTargetInfo, bool TaskTargetIsComplete)
{

}

void UTS_Task::TaskFinish_Implementation()
{
	//服务器上的任务计时是否结束了
	if (UKismetSystemLibrary::K2_GetTimerElapsedTimeHandle(this, TaskTimeHandle) >= TaskInfo.TaskTime)
	{
		bTaskIsFinish = true;//触发客户端的TaskEnd同步函数
	}

	//如果任务结束————停止计时器
	if (bTaskIsFinish)
	{
		GetWorld()->GetTimerManager().ClearTimer(TaskTimeHandle);
		//任务结束，停止所有的目标计时器
		for (TPair<FString, FTimerHandle> pair : TaskTargetTimeHandle)
		{
			GetWorld()->GetTimerManager().ClearTimer(pair.Value);
		}
	}
	TaskFinishEvent.Broadcast(this);
}

void UTS_Task::ServerTaskFinish(bool IsComplete)
{
	if (!bTaskIsFinish)//避免多次触发
	{
		bTaskIsComplete = IsComplete;
		bTaskIsFinish = true;//触发客户端的TaskFinish同步函数
		TaskFinish();//服务器调用该函数

		//任务完成时的 链接相关处理
		LinkTo(TaskInfo.TaskLinkInfo.LinkTo, IsComplete);

		TaskFinishToEnd(IsComplete);
	}
}

void UTS_Task::TaskFinishToEnd_Implementation(bool IsComplete)
{
	ServerTaskEnd(IsComplete);
}

void UTS_Task::TaskEnd_Implementation()
{
	TaskEndEvent.Broadcast(this);
}

void UTS_Task::TaskTimeEndBack_Implementation()
{
	ServerTaskEnd(TaskInfo.bTimeEndIsComplete);
}

void UTS_Task::ServerTaskEnd(bool IsComplete/* = true*/)
{
	if (!bTaskIsEnd)//避免多次触发
	{
		bTaskIsComplete = IsComplete;
		bTaskIsEnd = true;//触发客户端的TaskEnd同步函数
		TaskEnd();//服务器调用该函数

		//任务结束时的 链接相关处理
		//LinkTo(TaskInfo.TaskLinkInfo.LinkTo, IsComplete);
	}
}

void UTS_Task::TaskEndCheck()
{
	int32 CompleteTaskTargetNum = 0;//完成的任务目标数量
	int32 CompleteTaskTargetNum_MustDo = 0;//完成的必做任务目标数量
	int32 TaskTargetEndNum = 0;//结束的任务目标数量
	for (FTaskTargetInfo& Info : TaskInfo.TaskTargetInfo)
	{
		if (Info.bTaskTargetIsEnd)
		{
			TaskTargetEndNum++;
			if (Info.TaskTargetIsComplete())
			{
				if (Info.TaskTargetIsMustDo())//目标是否是必做
				{
					CompleteTaskTargetNum_MustDo++;
				}
				else
				{
					CompleteTaskTargetNum++;
				}
			}
		}
	}
	TaskEndCheckOfParameter(CompleteTaskTargetNum, CompleteTaskTargetNum_MustDo, TaskTargetEndNum);
}

void UTS_Task::TaskEndCheckOfParameter(int32 CompleteTaskTargetNum, int32 CompleteTaskTargetNum_MustDo, int32 TaskTargetEndNum)
{
	bool TaskIsComplete = CompleteTaskTargetNum_MustDo >= TaskInfo.TaskCompleteMustDoTargetNum;//必做目标判断
	if (TaskIsComplete)//普通目标判断
	{
		TaskIsComplete = CompleteTaskTargetNum >= TaskInfo.TaskCompleteTargetNum;
	}

	/*只有没有必做目标的任务才需要进行以下的判断
	* 任务完成了 || 如果任务没有完成，判断是否还有机会完成 （结束判断暂时屏蔽，无法完美解决情况2）
	* 情况1：任务目标在任务开始就全部发放了，这样判断是没有问题的
	* 情况2：任务目标需要在过程中动态给予，可能某个任务目标会触发新的目标，这样该判断就容易触发结束
	* 例如：需要完成6个目标的任务开始只发放了一个目标，那么这一个目标的更新，根据下面的判断始终会判断成：没有机会完成了
	*/
	if (TaskIsComplete)
	{
		//将全部目标设为结束
		for (FTaskTargetInfo& TargetInfo : TaskInfo.TaskTargetInfo)
		{
			TargetInfo.bTaskTargetIsEnd = true;//触发客户端的TaskUpdate同步函数
		}
		//ServerTaskEnd(TaskIsComplete);
		ServerTaskFinish(TaskIsComplete);
	}
}

void UTS_Task::ServerTaskTargetEnd_ID(int32 TaskTargetID, bool IsComplete)
{
	for (FTaskTargetInfo& Info : TaskInfo.TaskTargetInfo)
	{
		if (Info.TaskTargetID == TaskTargetID)
		{
			ServerTaskTargetEnd_TaskTargetInfo(Info, IsComplete);
			break;
		}
	}
}

void UTS_Task::ServerTaskTargetEnd_Index(int32 TaskTargetIndex, bool IsComplete)
{
	if (TaskInfo.TaskTargetInfo.IsValidIndex(TaskTargetIndex))
	{
		ServerTaskTargetEnd_TaskTargetInfo(TaskInfo.TaskTargetInfo[TaskTargetIndex], IsComplete);
	}
}

void UTS_Task::ServerTaskTargetEnd_TaskTargetInfo(UPARAM(Ref)FTaskTargetInfo& TaskTargetInfo, bool IsComplete)
{
	TaskTargetInfo.bTaskTargetIsEnd = true;//触发客户端的TaskUpdate同步函数
	if (IsComplete)
	{
		TaskTargetInfo.TaskTargetCurCount = TaskTargetInfo.TaskTargetMaxCount;
	}
	SomeTaskTargetEnd(TaskTargetInfo, IsComplete, GetLastRoleSignFromTaskTargetID(TaskTargetInfo.TaskTargetID));
	TaskTargetUpdate(TaskTargetInfo);
	TaskEndCheck();
}

void UTS_Task::ServerAddTaskTarget_Implementation(FTaskTargetInfo NewTaskTargetInfo)
{
	TaskInfo.TaskTargetInfo.Add(NewTaskTargetInfo);
	SetTaskTargetTimer(NewTaskTargetInfo);
	TaskTargetUpdate(NewTaskTargetInfo);//添加目标
}

void UTS_Task::ServerReSetTaskTime(float Time)
{
	if (TaskTime == Time)
	{
		TaskTime -= 0.000001;
	}
	else
	{
		TaskTime = Time;
	}

	//计时器只在服务器上开启
	if (TaskTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(TaskTimeHandle, this, &UTS_Task::TaskTimeEndBack, TaskTime);
	}

	TaskReSetTimeEvent.Broadcast(this);
}

void UTS_Task::SetTaskTargetTimer(FTaskTargetInfo& TaskTargetInfo)
{
	if (TaskTargetInfo.TaskTargetTime > 0.0f)//有时效显示的目标才会创建Handle
	{
		FTimerHandle TargetInfoTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TargetInfoTimerHandle, this, &UTS_Task::SomeTaskTargetTimeEnd, TaskTargetInfo.TaskTargetTime);
		TaskTargetTimeHandle.Add(TaskTargetInfo.TaskTargetText.ToString(), TargetInfoTimerHandle);
	}
}

void UTS_Task::ServerAddTaskTargetOfID(int32 NewTaskTargetID)
{
	FTaskTargetInfo Data, DTData;
	Data.TaskTargetID = NewTaskTargetID;
	if (TryInitTaskTargetFromDataTable(Data, DTData))
	{
		ServerAddTaskTarget(Data);
	}
}

void UTS_Task::ServerAddTaskTargetOfID_Array(TArray<int32> NewTaskTargetIDArray)
{
	for (int32& i : NewTaskTargetIDArray)
	{
		ServerAddTaskTargetOfID(i);
	}
}

void UTS_Task::SomeTaskTargetTimeEnd()
{
	//该函数被调用通常是某个任务目标时限到了 该函数在Server上才会被调用
	int32 CompleteTaskTargetNum = 0;//完成的任务目标数量
	int32 CompleteTaskTargetNum_MustDo = 0;//完成的必做任务目标数量
	int32 TaskTargetEndNum = 0;//结束的任务目标数量
	bool IsFind = false;
	TArray<FTaskTargetInfo> AllTaskTargetInfo = TaskInfo.TaskTargetInfo;//某个任务目标失败后可能会增加目标，为了防止在遍历中发生改变，这里复制一份数据出来进行遍历
	for (int32 i = 0; i < AllTaskTargetInfo.Num(); i++)
	{
		if (!IsFind)//找到结束的任务目标
		{
			FString Key = TaskInfo.TaskTargetInfo[i].TaskTargetText.ToString();
			if (TaskTargetTimeHandle.Contains(Key))
			{
				float TimerHandleTime = UKismetSystemLibrary::K2_GetTimerElapsedTimeHandle(this, TaskTargetTimeHandle[Key]);
				if (TimerHandleTime >= TaskInfo.TaskTargetInfo[i].TaskTargetTime || TimerHandleTime <= 0.0f)//<0表示已经结束了
				{
					TaskInfo.TaskTargetInfo[i].bTaskTargetIsEnd = true;//触发客户端的TaskUpdate同步函数
					if (TaskInfo.TaskTargetInfo[i].bTimeEndComplete)
					{
						TaskInfo.TaskTargetInfo[i].TaskTargetCurCount = TaskInfo.TaskTargetInfo[i].TaskTargetMaxCount;
					}
					SomeTaskTargetEnd(TaskInfo.TaskTargetInfo[i], TaskInfo.TaskTargetInfo[i].bTimeEndComplete, "None");
					TaskTargetUpdate(TaskInfo.TaskTargetInfo[i]);//服务器调用更新
					GetWorld()->GetTimerManager().ClearTimer(TaskTargetTimeHandle[Key]);
					TaskTargetTimeHandle.Remove(Key);
					IsFind = true;

				}
			}
		}
		if (TaskInfo.TaskTargetInfo[i].bTaskTargetIsEnd)
		{
			TaskTargetEndNum++;
			if (TaskInfo.TaskTargetInfo[i].TaskTargetIsComplete())
			{
				if (TaskInfo.TaskTargetInfo[i].TaskTargetIsMustDo())//目标是否是必做
				{
					CompleteTaskTargetNum_MustDo++;
				}
				else
				{
					CompleteTaskTargetNum++;
				}
			}
		}
	}

	TaskEndCheckOfParameter(CompleteTaskTargetNum, CompleteTaskTargetNum_MustDo, TaskTargetEndNum);
}

void UTS_Task::SomeTaskTargetEnd_Implementation(FTaskTargetInfo CompleteTaskTarget, bool IsComplete, FName RoleSign)
{
	//某个任务目标完成了，尝试触发添加连锁
	TriggerTaskChainAddInfo_Array(CompleteTaskTarget.ChainAddInfo, IsComplete ? ETS_TaskChainTriggerType::Complete : ETS_TaskChainTriggerType::Fail, RoleSign);

	//任务目标结束时的 链接相关处理
	LinkTo(CompleteTaskTarget.TaskTargetLinkInfo.LinkTo, IsComplete);

	FTaskTargetInfo TaskTargetInfo = CompleteTaskTarget;
	//目标在结束时是否要影响任务
	if (CompleteTaskTarget.bTaskTargetEndTask)
	{
		switch (CompleteTaskTarget.TaskTargetEndTaskType)
		{
		case ETS_LinkEndType::Follow:
		{
			//ServerTaskEnd(TaskTargetInfo.TaskTargetIsComplete());
			ServerTaskFinish(TaskTargetInfo.TaskTargetIsComplete());
			break;
		}
		case ETS_LinkEndType::Reverse:
		{
			//ServerTaskEnd(!TaskTargetInfo.TaskTargetIsComplete());
			ServerTaskFinish(!TaskTargetInfo.TaskTargetIsComplete());
			break;
		}
		case ETS_LinkEndType::AlwaysComplete:
		{
			//ServerTaskEnd(true);
			ServerTaskFinish(true);
			break;
		}
		case ETS_LinkEndType::AlwaysFail:
		{
			//ServerTaskEnd(false);
			ServerTaskFinish(false);
			break;
		}
		default:
			break;

		}
	}
	UE_LOG(LogTemp, Warning, TEXT("SomeTaskTargetEnd-FunctionEnd"));
}

bool UTS_Task::SomeTaskTargetUpdateCheck_Implementation(FTaskTargetInfo CheckTaskTarget, FRefreshTaskTargetInfo RefreshTaskTargetInfo)
{
	FText FailText;
	return CheckTaskTarget.BeCompareInfo.CompareResult(RefreshTaskTargetInfo.CompareInfo, FailText);
}

bool UTS_Task::GetTaskParameterValue(FName ParameterName, int32 Index, float& Value, FTaskParameter& Parameter)
{
	if (TaskParameter.Contains(ParameterName))
	{
		Parameter = TaskParameter[ParameterName];
		if (TaskParameter[ParameterName].Parameters.IsValidIndex(Index))
		{
			Value = TaskParameter[ParameterName].Parameters[Index];
		}
		return true;
	}
	else if (TaskInfo.GetParameterValue(ParameterName, Index, Value, Parameter))
	{
		TaskParameter.Add(ParameterName, Parameter);
		return true;
	}
	return false;
}

void UTS_Task::NetMultiChangeTaskMarkState(bool ShowOrHide)
{
	bool IsUpdate = bIsMarkTask != ShowOrHide;
	bIsMarkTask = ShowOrHide;
	if (IsUpdate)
	{
		TaskUpdate();
	}

	if (bIsMarkTask)
	{
		MarkTaskShow();
	}
	else
	{
		MarkTaskHide();
	}
}

void UTS_Task::RefreshTaskMarkInfo(TArray<AActor*> ActorInfos, TArray<FVector> LocationInfos)
{
	TaskMarkActors = ActorInfos;
	TaskMarkLocation = LocationInfos;

	if (bIsMarkTask)
	{
		MarkTaskShow();
	}

	TaskMarkInfoUpdate.Broadcast(TaskMarkActors, TaskMarkLocation);
}

void UTS_Task::GetTaskMarkInfo(TArray<AActor*>& ActorInfos, TArray<FVector>& LocationInfos)
{
	ActorInfos = TaskMarkActors;
	LocationInfos = TaskMarkLocation;
}

void UTS_Task::MarkTaskShow_Implementation()
{

}

void UTS_Task::MarkTaskHide_Implementation()
{

}

bool UTS_Task::GetTaskTargetFromID(int32 ID, FTaskTargetInfo*& TaskTargetInfo)
{
	for (FTaskTargetInfo& TaskTarget : TaskInfo.TaskTargetInfo)
	{
		if (TaskTarget.TaskTargetID == ID)
		{
			TaskTargetInfo = &TaskTarget;
			return true;
		}
	}
	return false;
}

void UTS_Task::TriggerTaskChainAddInfo(FTaskChainAddInfo TaskChainAddInfo, ETS_TaskChainTriggerType TriggerType, FName RoleSign)
{
	bool IsChain = false;
	switch (TaskChainAddInfo.TaskChainTriggerType)
	{
	case ETS_TaskChainTriggerType::Complete:
	{
		IsChain = (TriggerType == ETS_TaskChainTriggerType::Complete);
		break;
	}
	case ETS_TaskChainTriggerType::Fail:
	{
		IsChain = !(TriggerType == ETS_TaskChainTriggerType::Complete);
		break;
	}
	case ETS_TaskChainTriggerType::End:
	{
		IsChain = (TriggerType != ETS_TaskChainTriggerType::Active);
		break;
	}
	case ETS_TaskChainTriggerType::Active:
	{
		IsChain = (TriggerType == ETS_TaskChainTriggerType::Active);
	}
	default:
		break;
	}

	if (IsChain)
	{
		//如果LinkInfo中的影响源（BeLinkSourceHandle）未配置，那么该FTaskChainAddInfo.AddTaskRelatedHandle会赋予给LinkInfo中的影响源
		for (FTaskOneLinkInfo& OneLinkInfo : TaskChainAddInfo.LinkInfo.LinkTo)//我影响别人 影响源
		{
			if (OneLinkInfo.LinkSourceHandle.ID == -1)//-1未配置
			{
				OneLinkInfo.LinkSourceHandle = TaskChainAddInfo.AddTaskRelatedHandle;
			}
		}
		for (FTaskOneLinkInfo& OneLinkInfo : TaskChainAddInfo.LinkInfo.BeLink)//别人影响我
		{
			if (OneLinkInfo.LinkSourceHandle.ID == -1)//-1未配置
			{
				OneLinkInfo.LinkSourceHandle = TaskChainAddInfo.AddTaskRelatedHandle;
			}
		}

		switch (TaskChainAddInfo.AddTaskRelatedHandle.Type)
		{
		case ETS_TaskRelatedType::Task:
		{
			if (GetTS_GISubsystem())
			{
				//生成一个统一的任务
				UTS_Task* NewTask = GetTS_GISubsystem()->CreateTaskFromID(TaskChainAddInfo.AddTaskRelatedHandle.ID);
				if (NewTask)
				{
					//给任务赋予额外的链接信息
					NewTask->TaskInfo.TaskLinkInfo.AppendLinkInfo(TaskChainAddInfo.LinkInfo);
					for (UTS_TaskComponent*& TaskComponent : AllTaskComponent)
					{
						if (TaskComponent)
						{
							if (!TaskChainAddInfo.bAddTaskToTriggerRole)
							{
								TaskComponent->ServerAddTask(NewTask);
							}
							else if (TaskComponent->GetRoleSign() == RoleSign)
							{
								TaskComponent->ServerAddTask(NewTask);
								break;
							}
						}
					}
				}
			}
			break;
		}
		case ETS_TaskRelatedType::TaskTarget:
		{
			ServerAddTaskTargetOfID(TaskChainAddInfo.AddTaskRelatedHandle.ID);
			FTaskTargetInfo* TaskTargetInfo = nullptr;
			if (GetTaskTargetFromID(TaskChainAddInfo.AddTaskRelatedHandle.ID, TaskTargetInfo) && TaskTargetInfo)
			{
				TaskTargetInfo->TaskTargetLinkInfo.AppendLinkInfo(TaskChainAddInfo.LinkInfo);
			}
			break;
		}
		default:
			break;
		}
	}
}

void UTS_Task::TriggerTaskChainAddInfo_Array(TArray<FTaskChainAddInfo> TaskChainAddInfo, ETS_TaskChainTriggerType TriggerType, FName RoleSign)
{
	for (FTaskChainAddInfo& Info : TaskChainAddInfo)
	{
		TriggerTaskChainAddInfo(Info, TriggerType, RoleSign);
	}
}

UTS_GISubsystem* UTS_Task::GetTS_GISubsystem()
{
	if (!TS_GISubsystem)
	{
		UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
		if (GameInstance)
		{
			TS_GISubsystem = GameInstance->GetSubsystem<UTS_GISubsystem>();
		}
	}
	return TS_GISubsystem;
}

void UTS_Task::LinkTo(TArray<FTaskOneLinkInfo> LinkToInfo, bool IsComplete)
{
	//链接任务处理
	for (FTaskOneLinkInfo& OneLinkInfo : LinkToInfo)
	{
		bool LinkIsComplete = IsComplete;//影响的结果
		switch (OneLinkInfo.LinkEndType)
		{
		case ETS_LinkEndType::Follow:
		{
			LinkIsComplete = IsComplete;
			break;
		}
		case ETS_LinkEndType::Reverse:
		{
			LinkIsComplete = !IsComplete;
			break;
		}
		case ETS_LinkEndType::AlwaysComplete:
		{
			LinkIsComplete = true;
			break;
		}
		case ETS_LinkEndType::AlwaysFail:
		{
			LinkIsComplete = false;
			break;
		}
		default:
			break;
		}

		if (GetTS_GISubsystem())
		{
			FSameIDTask SameIDTask;
			//任务处理
			if (OneLinkInfo.ChainTaskHandle.ID != -1)
			{
				SameIDTask = GetTS_GISubsystem()->GetTaskFromID(OneLinkInfo.ChainTaskHandle.ID);//拿到要影响的任务
				if (OneLinkInfo.bIsLinkAllSameIDTask)//是否链接了全部同ID的任务
				{
					for (UTS_Task*& Task : SameIDTask.SameIDTask)
					{
						//Task->ServerTaskEnd(LinkIsComplete);
						Task->ServerTaskFinish(LinkIsComplete);
					}
				}
				else if (SameIDTask.Last())
				{
					//SameIDTask.Last()->ServerTaskEnd(LinkIsComplete);
					SameIDTask.Last()->ServerTaskFinish(LinkIsComplete);
				}
			}

			//任务目标
			if (OneLinkInfo.ChainTaskTargetHandle.ID != -1)
			{
				//如果影响源是任务，这里获取一下任务
				if (OneLinkInfo.LinkSourceHandle.Type == ETS_TaskRelatedType::Task)
				{
					SameIDTask = GetTS_GISubsystem()->GetTaskFromID(OneLinkInfo.LinkSourceHandle.ID);
				}

				if (OneLinkInfo.bIsLinkAllTaskTarget)//是否链接的是全部存在的该目标
				{
					TArray<UTS_Task*> AllTask;
					GetTS_GISubsystem()->AllTask_UniqueID.GenerateValueArray(AllTask);
					for (UTS_Task*& Task : AllTask)
					{
						Task->ServerTaskTargetEnd_ID(OneLinkInfo.ChainTaskTargetHandle.ID, LinkIsComplete);
					}
				}
				else if (SameIDTask.Last())
				{
					SameIDTask.Last()->ServerTaskTargetEnd_ID(OneLinkInfo.ChainTaskTargetHandle.ID, LinkIsComplete);
				}
			}
		}
	}
}

FName UTS_Task::GetLastRoleSignFromTaskTargetID(int32 TaskTargetID)
{
	FName RoleSign;
	if (TaskTargetRoleSign.Contains(TaskTargetID))
	{
		RoleSign = TaskTargetRoleSign[TaskTargetID];
	}
	return RoleSign;
}
