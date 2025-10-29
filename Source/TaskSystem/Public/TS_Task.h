// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <TS_StructAndEnum.h>
#include "Net/UnrealNetwork.h"
#include "TS_Task.generated.h"

class UTS_TaskComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTaskDelegate, UTS_Task*, Task);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTaskMarkInfoDelegate, const TArray<AActor*>&, TaskMarkActors, const TArray<FVector>&, TaskMarkLocation);//任务标记信息更新

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
	void ReplicatedUsing_CurUpdateTaskTarget();

	UFUNCTION()
	void ReplicatedUsing_bTaskIsStartChange();

	UFUNCTION()
	void ReplicatedUsing_bTaskIsEndChange();

	//通过信息刷新单个任务目标 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerRefreshOneTaskTargetFromInfo(FRefreshTaskTargetInfo RefreshTaskTargetInfo, UPARAM(Ref)FTaskTargetInfo& TaskTargetInfo);
	//通过信息刷新任务目标 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerRefreshTaskTargetFromInfo(FRefreshTaskTargetInfo RefreshTaskTargetInfo);
	//通过信息组刷新任务目标 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerRefreshTaskTargetFromInfoArray(const TArray<FRefreshTaskTargetInfo>& RefreshTaskTargetInfoArray);
	//通过组件刷新任务目标 该函数需要在服务器调用
	UFUNCTION(BlueprintCallable)
	void ServerRefreshTaskTargetFromComponent(UTS_TaskComponent* TriggerTaskComponent, FName RoleSign);

	/*尝试通过数据表初始化任务信息
	* DataTaskInfo：原始信息，主要使用TaskInfo.TaskID查表。在查询到同ID的数据后会进行覆盖
	* DTTaskInfo：查找到的表格中的原始信息
	* return：是否在数据表上找到了同ID的行数据
	*/
	UFUNCTION()
		bool TryInitTaskInfoFromDataTable(FTaskInfo& DataTaskInfo, FTaskInfo& DTTaskInfo);

	//在设置任务时的修改方案：可以通过该函数修改任务信息
	UFUNCTION(BlueprintNativeEvent)
	FTaskInfo ModifyTaskInfo(FTaskInfo DataTaskInfo);
		virtual FTaskInfo ModifyTaskInfo_Implementation(FTaskInfo DataTaskInfo);

	/*尝试通过数据表初始化任务目标
	* DataTaskTarget：原始信息，主要使用DataTaskTarget.TaskTargetID查表。在查询到同ID的数据后会进行覆盖
	* DTTaskTargetInfo：查找到的表格中的原始信息
	* return：是否在数据表上找到了同ID的行数据
	*/
	UFUNCTION()
		bool TryInitTaskTargetFromDataTable(FTaskTargetInfo& DataTaskTarget, FTaskTargetInfo& DTTaskTargetInfo);

	//在设置任务时的修改方案：可以通过该函数修改任务信息
	UFUNCTION(BlueprintNativeEvent)
	FTaskTargetInfo ModifyTaskTargetInfo(FTaskTargetInfo DataTaskTarget);
	virtual FTaskTargetInfo ModifyTaskTargetInfo_Implementation(FTaskTargetInfo DataTaskTarget);

	/*开始任务
	* 服务器客户端均会调用
	*/
	UFUNCTION(BlueprintNativeEvent)
		void StartTask();
	virtual void StartTask_Implementation();

	/*任务更新 TaskInfo更新时触发（任务数据本身发生改变，如任务结束、任务名称修改等），TaskTargetUpdate调用时该函数也会触发
	* 服务器客户端均会调用
	*/
	UFUNCTION(BlueprintNativeEvent)
		void TaskUpdate();
	virtual void TaskUpdate_Implementation();

	/*任务目标更新 通常是某个目标被刷新了（进度产生了变换） 新增/移除目标
	* UpdateTaskTarget：哪个目标导致的更新
	* 服务器客户端均会调用
	*/
	UFUNCTION(BlueprintNativeEvent)
	void TaskTargetUpdate(FTaskTargetInfo UpdateTaskTarget);
	virtual void TaskTargetUpdate_Implementation(FTaskTargetInfo UpdateTaskTarget);

	//新增任务目标的多播回调
	UFUNCTION(BlueprintNativeEvent)
	void NetMultiAddNewTaskTarget(FTaskTargetInfo NewTaskTarget);
	virtual void NetMultiAddNewTaskTarget_Implementation(FTaskTargetInfo NewTaskTarget);

	//任务目标结束的多播回调
	UFUNCTION(BlueprintNativeEvent)
	void NetMultiTaskTargetEnd(FTaskTargetInfo EndTaskTargetInfo, bool TaskTargetIsComplete);
	virtual void NetMultiTaskTargetEnd_Implementation(FTaskTargetInfo EndTaskTargetInfo, bool TaskTargetIsComplete);

	/*任务结束 目标全部完成 / 任务时间到了
	* 服务器客户端均会调用
	*/
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

	/*某个任务目标结束 该函数只在服务器调用
	* 调用情况1：时间自然结束了
	* 调用情况2：某个人的某个操作导致目标结束（完成/失败）
	* CompleteTaskTarget：结束的任务目标信息
	* IsComplete：结束时是否完成
	* RoleSign：哪个角色导致的结束，如果是自然时间结束该值也为None
	*/
	UFUNCTION(BlueprintNativeEvent)
		void SomeTaskTargetEnd(const FTaskTargetInfo& CompleteTaskTarget, bool IsComplete, FName RoleSign = "None");
	virtual void SomeTaskTargetEnd_Implementation(const FTaskTargetInfo& CompleteTaskTarget, bool IsComplete, FName RoleSign = "None");

	//某个任务目标更新的比对
	UFUNCTION(BlueprintNativeEvent)
		bool SomeTaskTargetUpdateCheck(FTaskTargetInfo CheckTaskTarget, FRefreshTaskTargetInfo RefreshTaskTargetInfo);
	virtual bool SomeTaskTargetUpdateCheck_Implementation(FTaskTargetInfo CheckTaskTarget, FRefreshTaskTargetInfo RefreshTaskTargetInfo);

	//获取任务参数
	UFUNCTION(BlueprintPure)
	bool GetTaskParameterValue(FName ParameterName,float& Value);

	/*改变任务标记状态，外部调用（如在任务界面点击追踪/标记）
	* ShowOrHide：该值为true时显示标记，该值为false时隐藏标记
	* 因为任务本身（Object）不支持RPC函数，这里利用任务组件进行多播
	*/
	UFUNCTION(BlueprintCallable)
	void NetMultiChangeTaskMarkState(bool ShowOrHide);

	/*刷新任务标记信息
	* 这里只是设置，在真正显示的时候获得该值进行显示
	* 无论是否需要显示标记，该函数都应该在合适的时机进行设置（如任务目标位置发生了改变）
	*/
	UFUNCTION(BlueprintCallable)
	void RefreshTaskMarkInfo(TArray<AActor*> ActorInfos, TArray<FVector> LocationInfos);

	//获取当前任务标记信息
	UFUNCTION(BlueprintPure)
	void GetTaskMarkInfo(TArray<AActor*>& ActorInfos, TArray<FVector>& LocationInfos);

	/*标记任务显示
	* 当ChangeTaskMarkState或SetTaskMarkInfo被调用时，若bIsShowTaskMark为true，该函数会被调用
	* 由子类去完成如何根据任务标记信息GetTaskMarkInfo()显示
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		void MarkTaskShow();
	virtual void MarkTaskShow_Implementation();

	/*标记任务隐藏
	* 当ChangeTaskMarkState被调用时，若bIsShowTaskMark为false，该函数会被调用
	* 当任务类被删除时也需要调用该函数会进行清除
	* 由子类去完成如何隐藏/删除任务标记显示
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		void MarkTaskHide();
	virtual void MarkTaskHide_Implementation();

	//通过ID获取该任务上的任务目标
	//UFUNCTION(BlueprintPure)
	bool GetTaskTargetFromID(int32 ID, FTaskTargetInfo*& TaskTargetInfo);
public:
	
	UPROPERTY(BlueprintAssignable)
		FTaskDelegate TaskStartEvent;
	UPROPERTY(BlueprintAssignable)
		FTaskDelegate TaskUpdateEvent;
	UPROPERTY(BlueprintAssignable)
		FTaskDelegate TaskEndEvent;

	//任务的唯一ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int32 TaskUniqueID;
	//任务信息
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, ReplicatedUsing = ReplicatedUsing_TaskInfoChange, Meta = (ExposeOnSpawn = True))
	FTaskInfo TaskInfo;
	//“上次”的全部任务目标
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTaskTargetInfo> LastAllTaskTargetInfo;

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

	//拥有该任务的全部组件
	UPROPERTY(BlueprintReadOnly)
		TArray<UTS_TaskComponent*> AllTaskComponent;

	//接取该任务的单位签名
	UPROPERTY(BlueprintReadWrite, Replicated)
		TArray<FName> RoleSigns;

	//任务时长
	UPROPERTY()
		FTimerHandle TaskTimeHandle;
	//该任务每个任务目标的时长Handle<目标名称,时长Handle>
	UPROPERTY()
		TMap<FString, FTimerHandle> TaskTargetTimeHandle;

	//任务标记显示状态
	UPROPERTY(BlueprintReadWrite, Replicated)
		bool bIsShowTaskMark = false;
	//任务标记的Actor信息
	UPROPERTY(BlueprintReadWrite, Replicated)
		TArray<AActor*> TaskMarkActors;
	//任务标记的位置信息
	UPROPERTY(BlueprintReadWrite, Replicated)
		TArray<FVector> TaskMarkLocation;

	UPROPERTY(BlueprintAssignable)
		FTaskMarkInfoDelegate TaskMarkInfoUpdate;

	//任务参数<参数名，参数>
	UPROPERTY(BlueprintReadWrite)
		TMap<FName,float> TaskParameter;

	/*当前导致TaskTargetUpdate触发的任务目标信息
	*/
	UPROPERTY(BlueprintReadWrite, Replicated, ReplicatedUsing = ReplicatedUsing_CurUpdateTaskTarget)
	TArray<FTaskTargetInfo> CurUpdateTaskTarget;

	//任务的连锁信息
	UPROPERTY(BlueprintReadWrite, Replicated)
	FTaskChainInfo TaskChainInfo;
};
