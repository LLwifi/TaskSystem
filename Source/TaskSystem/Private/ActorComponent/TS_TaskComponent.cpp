// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponent/TS_TaskComponent.h"
#include "Engine/ActorChannel.h"
#include <Common/TS_TaskConfig.h>
#include <Kismet/GameplayStatics.h>
#include <GameInstanceSubsystem/TS_GISubsystem.h>

// Sets default values for this component's properties
UTS_TaskComponent::UTS_TaskComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	if (TaskRole == ETS_TaskRole::Player)
	{
		SetIsReplicatedByDefault(true);
		// 开启 UE5 新版子对象同步列表机制
		bReplicateUsingRegisteredSubObjectList = true;
	}
	else
	{
		SetIsReplicatedByDefault(false);
		bReplicateUsingRegisteredSubObjectList = false;
	}

	// ...
}

#if WITH_EDITOR
void UTS_TaskComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	auto Property = PropertyChangedEvent.Property;//拿到改变的属性
	if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTS_TaskComponent, TaskRole))
	{
		if (TaskRole == ETS_TaskRole::Player)
		{
			SetIsReplicated(true);
			bReplicateUsingRegisteredSubObjectList = true;
		}
		else
		{
			SetIsReplicated(false);
			bReplicateUsingRegisteredSubObjectList = false;
		}
	}
}
#endif

// Called when the game starts
void UTS_TaskComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UTS_TaskComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTS_TaskComponent, AllTask);
}

bool UTS_TaskComponent::ReplicateSubobjects(class UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWrote = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	//手动同步任务类
	//for (UTS_Task*& Task : AllTask)
	//{
	//	if (!IsValid(Task))
	//	{
	//		continue;
	//	}
	//	bWrote |= Channel->ReplicateSubobject(Task, *Bunch, *RepFlags);
	//}

	//// 引擎底层会自动遍历 MyItemsArray，并对里面的每一个有效 UObject 调用同步逻辑
	//bWrote |= Channel->ReplicateSubobjectList(AllTask, *Bunch, *RepFlags);

	return bWrote;
}


// Called every frame
void UTS_TaskComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UTS_TaskComponent::ReplicatedUsing_AllTaskChange()
{
	AllTaskUpdateEvent.Broadcast(this);
}

FName UTS_TaskComponent::GetRoleSign()
{
	if (TaskRoleSign.IsNone())
	{
		return FName(UKismetSystemLibrary::GetDisplayName(this));
	}
	return TaskRoleSign;
}

void UTS_TaskComponent::SetTaskRole(ETS_TaskRole RoleType)
{
	TaskRole = RoleType;
	if (TaskRole == ETS_TaskRole::Player)
	{
		SetIsReplicated(true);
	}
	else
	{
		SetIsReplicated(false);
	}
}

void UTS_TaskComponent::ServerAddTaskTargetFromID_Implementation(int32 TaskID, int32 TaskTargetID)
{
	UTS_Task* Task;
	if (GetTaskOfID(TaskID, Task) && Task)
	{
		Task->ServerAddTaskTargetOfID(TaskTargetID);
	}
}

void UTS_TaskComponent::ServerAddTask_Implementation(UTS_Task* NewTask)
{
	if (NewTask)
	{
		NewTask->RoleSigns.Add(GetRoleSign());

		// 【关键步骤】将其注册到引擎组件的底层同步列表中
		// 底层会检查并利用 Push Model 机制将其纳入脏数据追踪范围
		AddReplicatedSubObject(NewTask);
		// 写入同步数组，触发引用同步
		AllTask.Add(NewTask);

		/*
		//后续删除时的顺序
		// 1. 先从同步数组中移除，切断引用链
		SubObjectsList.Remove(TargetObj);

		// 2. 【关键步骤】从组件的底层网络列表中注销
		// 这一步会安全关闭该子对象的 NetID 和对应的网络追踪，不再对其内部属性进行同步
		RemoveReplicatedSubObject(TargetObj);

		// 3. 释放逻辑层对该对象的强引用，使其能在下一个 GC 周期被安全回收
		// 注：无需手动调用 ConditionalBeginDestroy()，保持标准的 UObject 生命周期管理即可
		*/

		NewTask->AllTaskComponent.Add(this);
		NewTask->StartTask();//服务器调用该函数
		AddTaskEvent.Broadcast(this, NewTask);
	}
}

void UTS_TaskComponent::ServerAddTaskFromID_Implementation(int32 TaskID)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (GameInstance)
	{
		UTS_GISubsystem* TS_GISubsystem = GameInstance->GetSubsystem<UTS_GISubsystem>();
		if (TS_GISubsystem)
		{
			ServerAddTask(TS_GISubsystem->CreateTaskFromID(TaskID));
		}
	}
}

void UTS_TaskComponent::ServerAddTaskFromInfo_Implementation(FTaskInfo TaskInfo)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (GameInstance)
	{
		UTS_GISubsystem* TS_GISubsystem = GameInstance->GetSubsystem<UTS_GISubsystem>();
		if (TS_GISubsystem)
		{
			ServerAddTask(TS_GISubsystem->CreateTaskFromInfo(TaskInfo));
		}
	}
}

void UTS_TaskComponent::ServerFinishTaskFromID_Implementation(int32 TaskID, bool IsComplete)
{
	UTS_Task* Task;
	if (GetTaskOfID(TaskID, Task))
	{
		Task->ServerTaskFinish(IsComplete);
	}
}

void UTS_TaskComponent::ServerEndTaskFromID_Implementation(int32 TaskID, bool IsComplete)
{
	UTS_Task* Task;
	if (GetTaskOfID(TaskID, Task))
	{
		Task->ServerTaskEnd(IsComplete);
	}
}

void UTS_TaskComponent::ServerRefreshTaskTargetFromInfo_Implementation(FRefreshTaskTargetInfo RefreshTaskTargetInfo)
{
	TArray<UTS_Task*> TaskArray = AllTask;
	for (UTS_Task*& OneTask : TaskArray)//每个任务
	{
		if (!OneTask->bTaskIsComplete || !OneTask->bTaskIsEnd)//没有完成、没有结束的任务才会进行检测
		{
			OneTask->ServerRefreshTaskTargetFromInfo(RefreshTaskTargetInfo);
			if (OneTask->bTaskIsEnd)
			{
				TaskEndEvent.Broadcast(this, OneTask);
			}
		}
	}
}

void UTS_TaskComponent::ServerRefreshTaskTargetFromInfoArray_Implementation(const TArray<FRefreshTaskTargetInfo>& RefreshTaskTargetInfoArray)
{
	for (int32 i = 0; i < RefreshTaskTargetInfoArray.Num(); i++)
	{
		ServerRefreshTaskTargetFromInfo(RefreshTaskTargetInfoArray[i]);
	}
}

void UTS_TaskComponent::ServerRefreshTaskTargetFromComponent_Implementation(UTS_TaskComponent* TriggerTaskComponent, FName RoleSign)
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

bool UTS_TaskComponent::GetTaskOfID(int32 TaskID, UTS_Task*& Task)
{
	for (UTS_Task*& task : AllTask)
	{
		if (task && task->TaskInfo.TaskID == TaskID)
		{
			Task = task;
			return true;
		}
	}
	return false;
}

void UTS_TaskComponent::NetMultiTaskTargetUpdate_Implementation(UTS_Task* Task, FTaskTargetInfo UpdateTaskTarget)
{
	if (Task)
	{
		Task->TaskTargetUpdate(UpdateTaskTarget);
	}
}

void UTS_TaskComponent::ServerChangeTaskMarkStateFromTask_Implementation(UTS_Task* Task, bool ShowOrHide)
{
	if (Task)
	{
		NetMultiChangeTaskMarkStateFromTask(Task, ShowOrHide);
	}
}

void UTS_TaskComponent::NetMultiChangeTaskMarkStateFromTask_Implementation(UTS_Task* Task, bool ShowOrHide)
{
	if (Task)
	{
		bool IsUpdate = Task->bIsMarkTask != ShowOrHide;
		Task->NetMultiChangeTaskMarkState(ShowOrHide);
		if (IsUpdate)
		{
			Task->TaskUpdate();
		}
	}
}

void UTS_TaskComponent::ServerReSetTaskTimeFromTask_Implementation(UTS_Task* Task, float Time)
{
	if (Task)
	{
		Task->ServerReSetTaskTime(Time);
	}
}

