// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <TS_StructAndEnum.h>
#include "Net/UnrealNetwork.h"
#include "TS_Task.generated.h"

class UTS_TaskComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTaskDelegate, UTS_Task*, Task);

/**
 * 任务类
 * 任务类的Outer为UTS_TaskComponent才能网络同步
 */
UCLASS(Blueprintable, BlueprintType)
class TASKSYSTEM_API UTS_Task : public UObject
{
	GENERATED_BODY()

public:
	UTS_Task();

	virtual UWorld* GetWorld() const override;

	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	UFUNCTION()
	void ReplicatedUsing_TaskInfoChange();

	UFUNCTION()
	void ReplicatedUsing_bTaskIsStartChange();

	UFUNCTION()
	void ReplicatedUsing_bTaskIsEndChange();

	//通过信息刷新任务目标 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerRefreshTaskTargetFromInfo(FRefreshTaskTargetInfo RefreshTaskTargetInfo);
	//通过信息组刷新任务目标 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerRefreshTaskTargetFromInfoArray(const TArray<FRefreshTaskTargetInfo>& RefreshTaskTargetInfoArray);
	//通过组件刷新任务目标 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerRefreshTaskTargetFromComponent(UTS_TaskComponent* TriggerTaskComponent, FName RoleSign);

	//是否在数据表上找到了同ID的行数据 尝试通过数据表初始化任务信息（返回找到的行数据）
	UFUNCTION(BlueprintNativeEvent)
		bool TryInitTaskInfoFromDataTable(FTaskInfo& DTTaskInfo);
	virtual bool TryInitTaskInfoFromDataTable_Implementation(FTaskInfo& DTTaskInfo);

	//是否在数据表上找到了同ID的行数据 尝试通过数据表初始化任务目标（寻找所需的数据（主要是ID） 返回找到的行数据）
	UFUNCTION(BlueprintNativeEvent)
		bool TryInitTaskTargetFromDataTable(FTaskTargetInfo& TaskTarget, FTaskTargetInfo& DTTaskTargetInfo);
	virtual bool TryInitTaskTargetFromDataTable_Implementation(FTaskTargetInfo& TaskTarget, FTaskTargetInfo& DTTaskTargetInfo);

	//开始任务
	UFUNCTION(BlueprintNativeEvent)
		void StartTask();
	virtual void StartTask_Implementation();

	//任务更新 通常是某个目标的进度产生了变换 某个目标结束了 某个目标完成了
	UFUNCTION(BlueprintNativeEvent)
		void TaskUpdate();
	virtual void TaskUpdate_Implementation();

	//任务结束 目标全部完成 / 任务时间到了
	UFUNCTION(BlueprintNativeEvent)
		void TaskEnd();
	virtual void TaskEnd_Implementation();

	//主动使该任务结束 该函数需要在服务器调用 IsComplete：是否是完成结束该任务
	UFUNCTION(BlueprintCallable)
	void ServerTaskEnd(bool IsComplete = true);

	//任务结束检测
	UFUNCTION()
	void TaskEndCheck();
	//通过参数进行任务结束检测(完成的任务目标数量,完成的必做任务目标数量,结束的任务目标数量)
	UFUNCTION()
	void TaskEndCheckOfParameter(int32 CompleteTaskTargetNum, int32 CompleteTaskTargetNum_MustDo, int32 TaskTargetEndNum);

	//主动使该任务的某个目标结束 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerTaskTargetEnd_ID(int32 TaskTargetID, bool IsComplete = true);
	//主动使该任务的某个目标结束 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerTaskTargetEnd_Index(int32 TaskTargetIndex, bool IsComplete = true);
	//主动使该任务的某个目标结束 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerTaskTargetEnd_TaskTargetInfo(UPARAM(Ref)FTaskTargetInfo& TaskTargetInfo, bool IsComplete = true);

	//动态给任务新增一个目标 该函数需要在服务器调用
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ServerAddTaskTarget(FTaskTargetInfo NewTaskTargetInfo);
	virtual void ServerAddTaskTarget_Implementation(FTaskTargetInfo NewTaskTargetInfo);

	//根据任务目标信息设置计时器
	UFUNCTION()
	void SetTaskTargetTimer(FTaskTargetInfo& TaskTargetInfo);

	//动态通过任务目标ID给任务新增一个目标 该函数需要在服务器调用 该函数同样会触发ServerAddTaskTarget
	UFUNCTION(BlueprintCallable)
	void ServerAddTaskTargetOfID(int32 NewTaskTargetID);

	UFUNCTION(BlueprintCallable)
	void ServerAddTaskTargetOfID_Array(TArray<int32> NewTaskTargetIDArray);

	//某个任务目标计时结束 该函数只在服务器调用
	UFUNCTION()
		void SomeTaskTargetTimeEnd();

	//某个任务目标结束 该函数只在服务器调用
	UFUNCTION(BlueprintNativeEvent)
		void SomeTaskTargetEnd(const FTaskTargetInfo& CompleteTaskTarget, bool IsComplete);
	virtual void SomeTaskTargetEnd_Implementation(const FTaskTargetInfo& CompleteTaskTarget, bool IsComplete);

	/*某个任务目标更新的二次检测（该任务目标的信息，触发这次更新的任务目标信息）
	* 当一个非ID标识 的任务目标tag比对成功时，该函数会被调用：用于二次判断该任务目标是否可以增加进度
	* 该函数默认实现了Class/Tag/String的对比
	*/
	UFUNCTION(BlueprintNativeEvent)
		bool SomeTaskTargetUpdateCheck(FTaskTargetInfo CheckTaskTarget, FRefreshTaskTargetInfo RefreshTaskTargetInfo);
	virtual bool SomeTaskTargetUpdateCheck_Implementation(FTaskTargetInfo CheckTaskTarget, FRefreshTaskTargetInfo RefreshTaskTargetInfo);

	//获取任务参数
	UFUNCTION(BlueprintPure)
	bool GetTaskParameterValue(FName ParameterName,float& Value);
public:
	
	UPROPERTY(BlueprintAssignable)
		FTaskDelegate TaskStartEvent;
	UPROPERTY(BlueprintAssignable)
		FTaskDelegate TaskUpdateEvent;
	UPROPERTY(BlueprintAssignable)
		FTaskDelegate TaskEndEvent;

	//任务信息
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, ReplicatedUsing = ReplicatedUsing_TaskInfoChange, Meta = (ExposeOnSpawn = True))
		FTaskInfo TaskInfo;
	//任务的唯一ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
		int32 TaskUniqueID;

	//任务拥有必做目标的数量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
		int32 TaskMustDoTargetNum = 0;
	//任务是否完成
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
		bool bTaskIsComplete = false;
	//任务是否开始
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = ReplicatedUsing_bTaskIsStartChange)
		bool bTaskIsStart = false;
	//任务是否结束
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = ReplicatedUsing_bTaskIsEndChange)
		bool bTaskIsEnd = false;

	UPROPERTY(BlueprintReadOnly)
		UTS_TaskComponent* TaskComponent;

	//接取该任务的单位签名
	UPROPERTY(BlueprintReadWrite, Replicated)
		TArray<FName> RoleSigns;

	//任务时长
	UPROPERTY()
		FTimerHandle TaskTimeHandle;
	//该任务每个任务目标的时长Handle<目标名称,时长Handle>
	UPROPERTY()
		TMap<FString, FTimerHandle> TaskTargetTimeHandle;

	//任务相关单位 （显示距离等等信息）
	UPROPERTY(BlueprintReadWrite, Replicated)
		TArray<AActor*> TaskActor;

	//任务参数<参数名，参数>
	UPROPERTY(BlueprintReadWrite)
		TMap<FName,float> TaskParameter;
};
