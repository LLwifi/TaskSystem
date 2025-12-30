// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <GameplayTagContainer.h>
#include "TS_Task.h"
#include "Net/UnrealNetwork.h"
#include "TS_TaskComponent.generated.h"

//任务角色
UENUM(BlueprintType)
enum class ETS_TaskRole :uint8
{
	None = 0 UMETA(DisplayName = "无"),
	Player UMETA(DisplayName = "Player-做任务的单位"),
	NPC UMETA(DisplayName = "NPC-发任务的单位"),
	People UMETA(DisplayName = "People-与任务相关的单位")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTaskComponentDelegate, UTS_TaskComponent*, AllTaskComponent,  UTS_Task*, Task);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTaskUpdateDelegate, UTS_TaskComponent*, TaskComponent);

/*任务组件
* 处理任务
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TASKSYSTEM_API UTS_TaskComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTS_TaskComponent();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void ReplicatedUsing_AllTaskChange();

	//获取签名
	UFUNCTION(BlueprintPure)
	FName GetRoleSign();

	//设置任务角色
	UFUNCTION(BlueprintCallable)
		void SetTaskRole(ETS_TaskRole RoleType);

	//添加任务
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ServerAddTask(UTS_Task* NewTask, FTaskChainInfo TaskChainInfo = FTaskChainInfo());

	//通过任务ID添加任务
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ServerAddTaskFromID(int32 TaskID, FTaskChainInfo TaskChainInfo = FTaskChainInfo());

	//通过任务信息添加任务
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ServerAddTaskFromInfo(FTaskInfo TaskInfo, FTaskChainInfo TaskChainInfo = FTaskChainInfo());

	//结束任务（任务ID 是否属于完成结束）
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ServerEndTaskFromID(int32 TaskID, bool IsComplete);

	//通过信息刷新任务目标
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ServerRefreshTaskTargetFromInfo(FRefreshTaskTargetInfo RefreshTaskTargetInfo);
	//通过信息组刷新任务目标
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ServerRefreshTaskTargetFromInfoArray(const TArray<FRefreshTaskTargetInfo>& RefreshTaskTargetInfoArray);
	//通过组件刷新任务目标
	UFUNCTION(BlueprintCallable, Server, Reliable)
		void ServerRefreshTaskTargetFromComponent(UTS_TaskComponent* TriggerTaskComponent, FName RoleSign);

	//通过ID获取当前接取的任务
	UFUNCTION(BlueprintPure)
		bool GetTaskOfID(int32 TaskID, UTS_Task*& Task);
		
	/*多播 通过任务改变标记状态
	* 因为任务本身（Object）不支持RPC函数，这里利用组件进行多播
	*/
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	void NetMultiChangeTaskMarkStateFromTask(UTS_Task* Task, bool ShowOrHide);
public:

	UPROPERTY(BlueprintAssignable)
	FTaskComponentDelegate AddTaskEvent;
	UPROPERTY(BlueprintAssignable)
	FTaskComponentDelegate TaskEndEvent;
	UPROPERTY(BlueprintAssignable)
	FTaskUpdateDelegate AllTaskUpdateEvent;

	//该Actor在任务流程中扮演什么样的角色
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		ETS_TaskRole TaskRole = ETS_TaskRole::People;

	//该Actor的角色签名（角色唯一标识-通常是ID/或者玩家ID）在任务贡献中通过该值区分不同角色提供的贡献
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName TaskRoleSign = "None";

	//全部任务
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, ReplicatedUsing = ReplicatedUsing_AllTaskChange)
		TArray<UTS_Task*> AllTask;

	//与“我”相关的任务目标刷新信息
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FRefreshTaskTargetInfo> RefreshTaskTargetInfos;
};
