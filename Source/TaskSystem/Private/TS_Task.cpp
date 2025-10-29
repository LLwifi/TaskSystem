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
			UE_LOG(LogTemp, Warning, TEXT("Task Outer Must Be UTS_TaskComponent Then Can RPC"));
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
	DOREPLIFETIME(UTS_Task, TaskUniqueID);
	DOREPLIFETIME(UTS_Task, TaskMustDoTargetNum);
	DOREPLIFETIME(UTS_Task, bTaskIsComplete);
	DOREPLIFETIME(UTS_Task, bTaskIsStart);
	DOREPLIFETIME(UTS_Task, bTaskIsEnd);
	DOREPLIFETIME(UTS_Task, RoleSigns);

	DOREPLIFETIME(UTS_Task, bIsShowTaskMark);
	DOREPLIFETIME(UTS_Task, TaskMarkActors);
	DOREPLIFETIME(UTS_Task, TaskMarkLocation);
	DOREPLIFETIME(UTS_Task, CurUpdateTaskTarget);
	DOREPLIFETIME(UTS_Task, TaskChainInfo);
}

void UTS_Task::ReplicatedUsing_TaskInfoChange()
{
	TaskUpdate();
}

void UTS_Task::ReplicatedUsing_CurUpdateTaskTarget()
{
	//客户端的相关函数在同步后调用
	TArray<FTaskTargetInfo> UpdateTaskTargetArray = CurUpdateTaskTarget;
	for (FTaskTargetInfo& TaskTargetInfo : UpdateTaskTargetArray)
	{
		TaskTargetUpdate(TaskTargetInfo);
	}
	CurUpdateTaskTarget.Empty();
}

void UTS_Task::ReplicatedUsing_bTaskIsStartChange()
{
	StartTask();
}

void UTS_Task::ReplicatedUsing_bTaskIsEndChange()
{
	TaskEnd();
}

void UTS_Task::ServerRefreshOneTaskTargetFromInfo(FRefreshTaskTargetInfo RefreshTaskTargetInfo, UPARAM(Ref)FTaskTargetInfo& TaskTargetInfo)
{
	if (!TaskTargetInfo.bTaskTargetIsEnd && (RefreshTaskTargetInfo.RefreshTaskTargetID == TaskTargetInfo.TaskTargetID ||//ID相同直接通过
		(TaskTargetInfo.bCompareInfoIsOverride && SomeTaskTargetUpdateCheck(TaskTargetInfo, RefreshTaskTargetInfo))))//开启除了ID之外的其他检测方式 && 经过自定义的二次比对后可以增加进度
	{
		//通过比对
		if (TaskTargetInfo.AddProgress(RefreshTaskTargetInfo.AddProgress, RefreshTaskTargetInfo.RoleSign))//触发客户端的TaskUpdate同步函数
		{
			SomeTaskTargetEnd(TaskTargetInfo, true, RefreshTaskTargetInfo.RoleSign);
		}
		TaskTargetUpdate(TaskTargetInfo);//服务器调用该函数
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

void UTS_Task::StartTask_Implementation()
{
	if (UKismetSystemLibrary::IsServer(this))//该函数首次由UTS_TaskComponent在服务器调用
	{
		FTaskInfo TaskInfoTemp;
		TryInitTaskInfoFromDataTable(TaskInfo, TaskInfoTemp);//查表初始化

		//计时器只在服务器上开启
		if (TaskInfo.TaskTime > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(TaskTimeHandle, this, &UTS_Task::TaskEnd, TaskInfo.TaskTime);
		}

		TArray<FTaskTargetInfo> InitTaskTarget = TaskInfo.TaskTargetInfo;
		TaskInfo.TaskTargetInfo.Empty();//清空，通过下面for循环处理完后再重新添加
		for (FTaskTargetInfo& TaskTargetInfo : InitTaskTarget)
		{
			SetTaskTargetTimer(TaskTargetInfo);

			ServerAddTaskTarget(TaskTargetInfo);
		}

		bTaskIsStart = true;//触发客户端的StartTask同步函数
	}
	TaskStartEvent.Broadcast(this);
}

void UTS_Task::TaskUpdate_Implementation()
{
	TaskUpdateEvent.Broadcast(this);
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

void UTS_Task::TaskEnd_Implementation()
{
	//服务器上的任务计时是否结束了
	if (UKismetSystemLibrary::K2_GetTimerElapsedTimeHandle(this, TaskTimeHandle) >= TaskInfo.TaskTime)
	{
		bTaskIsEnd = true;//触发客户端的TaskEnd同步函数
	}

	//如果任务结束————停止计时器
	if (bTaskIsEnd)
	{
		GetWorld()->GetTimerManager().ClearTimer(TaskTimeHandle);
		//任务结束，停止所有的目标计时器
		for (TPair<FString, FTimerHandle> pair : TaskTargetTimeHandle)
		{
			GetWorld()->GetTimerManager().ClearTimer(pair.Value);
		}
	}
	TaskEndEvent.Broadcast(this);
}

void UTS_Task::ServerTaskEnd(bool IsComplete/* = true*/)
{
	if (!bTaskIsEnd)//避免多次触发
	{
		bTaskIsComplete = IsComplete;
		bTaskIsEnd = true;//触发客户端的TaskEnd同步函数
		TaskEnd();//服务器调用该函数

		//连锁任务处理
		for (FTaskOnceChainInfo& OneChainInfo : TaskChainInfo.AllTaskChainInfo)
		{
			for (UTS_TaskComponent*& TSCom : AllTaskComponent)//拥有“我”这个任务的全部组件
			{
				UTS_Task* ChainTask;
				if (TSCom->GetTaskOfID(OneChainInfo.ChainTaskHandle.ID, ChainTask) && ChainTask)
				{
					//找到任务后判断是影响任务本身还是任务上的某个目标
					FTaskTargetInfo* ChainTaskTargetInfo;
					bool ChainIsComplete = bTaskIsComplete;//影响的结果
					if (OneChainInfo.ChainTaskTargetHandle.ID == -1)//影响任务
					{
						switch (OneChainInfo.ChainEndType)
						{
						case ETS_ChainEndType::Follow:
						{
							ChainIsComplete = bTaskIsComplete;
							break;
						}
						case ETS_ChainEndType::Reverse:
						{
							ChainIsComplete = !bTaskIsComplete;
							break;
						}
						case ETS_ChainEndType::AlwaysComplete:
						{
							ChainIsComplete = bTaskIsEnd;
							break;
						}
						case ETS_ChainEndType::AlwaysFail:
						{
							ChainIsComplete = !bTaskIsEnd;
							break;
						}
						default:
							break;
						}
						ChainTask->ServerTaskEnd(ChainIsComplete);
					}
					else if(ChainTask->GetTaskTargetFromID(OneChainInfo.ChainTaskTargetHandle.ID, ChainTaskTargetInfo))//影响任务上的某个目标
					{
						FRefreshTaskTargetInfo RefreshInfo;
						RefreshInfo.RefreshTaskTargetID = OneChainInfo.ChainTaskTargetHandle.ID;
						RefreshInfo.AddProgress = ChainTaskTargetInfo->TaskTargetMaxCount;
						for (UTS_TaskComponent*& Com :AllTaskComponent)//获取一个有效的任务签名
						{
							if (Com)
							{
								RefreshInfo.RoleSign = Com->GetRoleSign();
								break;
							}
						}
						ChainTask->ServerRefreshOneTaskTargetFromInfo(RefreshInfo, *ChainTaskTargetInfo);
					}
				}
			}
		}
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
	bool TaskIsComplete = false;
	if (TaskMustDoTargetNum > 0)//任务是否有必做目标
	{
		TaskIsComplete = CompleteTaskTargetNum_MustDo >= TaskMustDoTargetNum;
	}
	else
	{
		TaskIsComplete = CompleteTaskTargetNum + CompleteTaskTargetNum_MustDo >= TaskInfo.TaskCompleteTargetNum;
	}

	/*只有没有必做目标的任务才需要进行以下的判断
	* 任务完成了 || 如果任务没有完成，判断是否还有机会完成 （结束判断暂时屏蔽，无法完美解决情况2）
	* 情况1：任务目标在任务开始就全部发放了，这样判断是没有问题的
	* 情况2：任务目标需要在过程中动态给予，可能某个任务目标会触发新的目标，这样该判断就容易触发结束
	* 例如：需要完成6个目标的任务开始只发放了一个目标，那么这一个目标的更新，根据下面的判断始终会判断成：没有机会完成了
	*/
	if (TaskIsComplete/* || (TaskMustDoTargetNum <= 0 && TaskInfo.TaskTargetInfo.Num() - TaskTargetEndNum + CompleteTaskTargetNum + CompleteTaskTargetNum_MustDo < TaskInfo.TaskCompleteTargetNum)*/)
	{
		//将全部目标设为结束
		for (FTaskTargetInfo& TargetInfo : TaskInfo.TaskTargetInfo)
		{
			TargetInfo.bTaskTargetIsEnd = true;//触发客户端的TaskUpdate同步函数
		}
		ServerTaskEnd(TaskIsComplete);
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
	TaskEndCheck();
}

void UTS_Task::ServerAddTaskTarget_Implementation(FTaskTargetInfo NewTaskTargetInfo)
{
	TaskInfo.TaskTargetInfo.Add(NewTaskTargetInfo);
	if (NewTaskTargetInfo.TaskTargetIsMustDo())
	{
		TaskMustDoTargetNum++;
	}
	SetTaskTargetTimer(NewTaskTargetInfo);
	TaskTargetUpdate(NewTaskTargetInfo);//添加目标
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
	FTaskTargetInfo Data,DTData;
	Data.TaskTargetID = NewTaskTargetID;
	if (TryInitTaskTargetFromDataTable(Data, DTData))
	{
		ServerAddTaskTarget(DTData);
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

void UTS_Task::SomeTaskTargetEnd_Implementation(const FTaskTargetInfo& CompleteTaskTarget, bool IsComplete, FName RoleSign)
{
	//某个任务目标完成了，尝试触发连锁
	for (FTaskChainAddInfo ChainAddInfo : CompleteTaskTarget.ChainAddInfo)
	{
		ChainAddInfo.AllChainInfo.AutoSetID(TaskInfo.TaskID, CompleteTaskTarget.TaskTargetID);//尝试刷新ID数据
		bool IsChain = false;
		switch (ChainAddInfo.TaskChainTriggerType)
		{
		case ETS_TaskChainTriggerType::Complete:
		{
			IsChain = IsComplete;
			break;
		}
		case ETS_TaskChainTriggerType::Fail:
		{
			IsChain = !IsComplete;
			break;
		}
		case ETS_TaskChainTriggerType::End:
		{
			IsChain = true;
			break;
		}
		default:
			break;
		}

		if (IsChain)
		{
			switch (ChainAddInfo.AddTaskRelatedHandle.Type)
			{
			case ETS_TaskRelatedType::Task:
			{
				//如果触发者为None 只要添加任务是给与任务相关的所有人那么还可以继续
				bool IsAddNewTask = RoleSign.IsNone() ? !ChainAddInfo.bAddTaskToTriggerRole : true;
				if (IsAddNewTask)
				{
					UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
					if (GameInstance)
					{
						UTS_GISubsystem* TS_GISubsystem = GameInstance->GetSubsystem<UTS_GISubsystem>();
						if (TS_GISubsystem)
						{
							//生成一个统一的任务
							UTS_Task* NewTask = TS_GISubsystem->CreateTaskFromID(ChainAddInfo.AddTaskRelatedHandle.ID);
							if (NewTask)
							{
								for (UTS_TaskComponent*& TaskComponent : AllTaskComponent)
								{
									if (TaskComponent)
									{
										if (!ChainAddInfo.bAddTaskToTriggerRole)
										{
											TaskComponent->ServerAddTask(NewTask, ChainAddInfo.AllChainInfo);
										}
										else if (TaskComponent->GetRoleSign() == RoleSign)
										{
											TaskComponent->ServerAddTask(NewTask, ChainAddInfo.AllChainInfo);
											break;
										}
									}
								}
							}
						}
					}
				}
				break;
			}
			case ETS_TaskRelatedType::TaskTarget:
			{
				ServerAddTaskTargetOfID(ChainAddInfo.AddTaskRelatedHandle.ID);
				break;
			}
			default:
				break;
			}
		}
	}

	FTaskTargetInfo TaskTargetInfo = CompleteTaskTarget;
	//目标在结束时是否要影响任务
	if (CompleteTaskTarget.bTaskTargetEndTask)
	{
		switch (CompleteTaskTarget.TaskTargetEndTaskType)
		{
		case ETS_ChainEndType::Follow:
		{
			ServerTaskEnd(TaskTargetInfo.TaskTargetIsComplete());
			break;
		}
		case ETS_ChainEndType::Reverse:
		{
			ServerTaskEnd(!TaskTargetInfo.TaskTargetIsComplete());
			break;
		}
		case ETS_ChainEndType::AlwaysComplete:
		{
			ServerTaskEnd(true);
			break;
		}
		case ETS_ChainEndType::AlwaysFail:
		{
			ServerTaskEnd(false);
			break;
		}
		default:
			break;

		}
	}
}

bool UTS_Task::SomeTaskTargetUpdateCheck_Implementation(FTaskTargetInfo CheckTaskTarget, FRefreshTaskTargetInfo RefreshTaskTargetInfo)
{
	FText FailText;
	return CheckTaskTarget.BeCompareInfo.CompareResult(RefreshTaskTargetInfo.CompareInfo, FailText);
}

bool UTS_Task::GetTaskParameterValue(FName ParameterName, float& Value)
{
	if (TaskParameter.Contains(ParameterName))
	{
		Value = TaskParameter[ParameterName];
		return true;
	}
	else if(TaskInfo.GetParameterValue(ParameterName, Value))
	{
		TaskParameter.Add(ParameterName, Value);
		return true;
	}
	return false;
}

void UTS_Task::NetMultiChangeTaskMarkState(bool ShowOrHide)
{
	bIsShowTaskMark = ShowOrHide;
	if (bIsShowTaskMark)
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

	if (bIsShowTaskMark)
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