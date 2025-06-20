#pragma once

#include "CoreMinimal.h"
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>
#include <Common/TS_TaskConfig.h>
#include "TS_StructAndEnum.generated.h"

class UTS_TaskCompare;

/*任务的对比对照结构体
* 该结构体将常见的比较情况进行了汇总
*/
USTRUCT(BlueprintType)
struct FTaskCompareInfo : public FTableRowBase
{
	GENERATED_BODY()
public:
	//比对方式是否使用任务比对类
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsUseTaskCompare = false;

	//任务对照类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditConditionHides, EditCondition = "IsUseTaskCompare"))
	TSoftClassPtr<UTS_TaskCompare> TaskCompareClass;

	/*TaskCompareTag_Info，与外部进行比对时的决策
	* 该值为Flase时，外部满足任意一个Tag即可
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditConditionHides, EditCondition = "!IsUseTaskCompare"))
	bool TaskCompareTagIsAllMatch = true;

	/*TaskCompareTag_Info，与外部进行比对时的精准决策
	* 该值为Flase时，外部Tag包含父类即可
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditConditionHides, EditCondition = "!IsUseTaskCompare"))
	bool TaskCompareTagIsExactMatch = true;

	/*任务对照Tag
	* 可以作为类型使用，例如击杀/收集任务（任务目标类型对照） 等级/职业判断（任务条件类型对照）
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Task"), meta = (EditConditionHides, EditCondition = "!IsUseTaskCompare"))
	FGameplayTagContainer TaskCompareTag_Info;

	/*任务对照Class 会判断是否等于该类或该类的子类
	* 该值用来判断目标的类型，例如判断击杀/交互的单位
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditConditionHides, EditCondition = "!IsUseTaskCompare"))
	TSoftClassPtr<UObject> TaskCompareClass_Info;

	/*任务对照自定义信息 自定义字符比对，任务目标检测时额外的比对项目
	* 某些不至于使用UObject但是Class不足以判断内容时，可以选择使用该内容进行判断
	* 例如：击杀的目标是否携带某种状态也可以通过定于该值进行判断：例如用Fire代表燃烧；Ice代表冰冻（任务目标类型对照）
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditConditionHides, EditCondition = "!IsUseTaskCompare"))
	FString TaskCompareString_Info;

	/*任务对照Obejct信息
	* 该值用来判断一些更加细致的具体事项（例如复数信息），例如击杀的目标是否携带某种状态，交互的单位身价是否超过某个数值
	*/
	UPROPERTY(BlueprintReadWrite)
	UObject* TaskCompareObject_Info;
};

/*任务的前置条件，是否可以接取该任务
*/
USTRUCT(BlueprintType)
struct FTaskConditionInfo : public FTableRowBase
{
	GENERATED_BODY()
public:
	//任务前置条件ID 任务前置条件ID和任务前置条件ID之间唯一，不可重复
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TaskConditionID;

	//任务目标对照结构信息
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTaskCompareInfo TaskConditionCompareInfo;
};

/*任务的贡献
*/
USTRUCT(BlueprintType)
struct FTaskContribute
{
	GENERATED_BODY()
public:
	FTaskContribute(){}
	FTaskContribute(FName _RoleSign, int32 _Progress)
	{
		RoleSign = _RoleSign;
		Progress = _Progress;
	}
public:
	//角色签名-谁提供的贡献（ID、名称都可以）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RoleSign;

	//贡献值
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Progress;
};

//全部角色的贡献
USTRUCT(BlueprintType)
struct FTaskRoleContribute
{
	GENERATED_BODY()
public:
	//添加贡献
	void AddContribute(FName RoleSign, int32 Progress)
	{
		for (FTaskContribute& Contribute : RoleContribute)
		{
			if (Contribute.RoleSign == RoleSign)
			{
				Contribute.Progress += Progress;
				return;
			}
		}
		RoleContribute.Add(FTaskContribute(RoleSign, Progress));
	}
	//通过角色签名获取贡献
	int32 GetContributeFromRoleSign(FName& RoleSign)
	{
		for (FTaskContribute& Contribute : RoleContribute)
		{
			if (Contribute.RoleSign == RoleSign)
			{
				return Contribute.Progress;
			}
		}
		return 0;
	}
public:
	//全部角色的贡献
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTaskContribute> RoleContribute;
};

/*任务的参数
*/
USTRUCT(BlueprintType)
struct FTaskParameter
{
	GENERATED_BODY()
public:
	FTaskParameter() {}
	FTaskParameter(FName _Name, int32 _Value)
	{
		Name = _Name;
		Value = _Value;
	}
public:
	//参数名
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

	//参数值
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value;
};

/*任务连锁信息
* 在任务/任务目标完成/失败时通过该信息自动补充内容（任务/任务目标）
*/
USTRUCT(BlueprintType)
struct FTaskChainInfo
{
	GENERATED_BODY()
public:
	/*该连锁信息是在完成时触发吗
	* 该值为false时在失败是触发
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTriggerIsComplete = true;

	//类型Tag  是任务还是任务目标
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Task"))
	FGameplayTag TypeTag = FGameplayTag::RequestGameplayTag("Task.Target");

	//要触发的内容（任务/任务目标）ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ID;
};

/*任务目标
*/
USTRUCT(BlueprintType)
struct FTaskTargetInfo : public FTableRowBase
{
	GENERATED_BODY()
public:
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override
	{
		FTaskTargetInfo* TaskTargetInfo = InDataTable->FindRow<FTaskTargetInfo>(InRowName, TEXT(""));
		if (TaskTargetInfo && InRowName.ToString().IsNumeric())
		{
			TaskTargetInfo->TaskTargetID = FCString::Atoi(*InRowName.ToString());
		}
	}

	//通过其他任务目标信息初始化自身 通常是通过表格内的数据初始化自身
	void InitFromTaskTargetInfo(FTaskTargetInfo& TaskTargetInfo)
	{
		TaskTargetID = TaskTargetInfo.TaskTargetID;
		//某些属性可能会被覆盖，不使用表格数据
		if (TaskTargetTime == -1.0f)
		{
			TaskTargetTime = TaskTargetInfo.TaskTargetTime;
		}
		bTimeEndComplete = TaskTargetInfo.bTimeEndComplete;
		if (TaskTargetText.IsEmpty())
		{
			TaskTargetText = TaskTargetInfo.TaskTargetText;
		}
		if (TaskTargetTag.Num() == 0)
		{
			TaskTargetTag = TaskTargetInfo.TaskTargetTag;
		}
		bTaskTargetIsEnd = TaskTargetInfo.bTaskTargetIsEnd;
		TaskTargetCurCount = TaskTargetInfo.TaskTargetCurCount;
		if (TaskTargetMaxCount == -1)
		{
			TaskTargetMaxCount = TaskTargetInfo.TaskTargetMaxCount;
		}
		RoleContribute = TaskTargetInfo.RoleContribute;
		if (!bCompareInfoIsOverride)
		{
			bCompareInfoIsOverride = TaskTargetInfo.bCompareInfoIsOverride;
			TaskTargetCompareInfo = TaskTargetInfo.TaskTargetCompareInfo;
		}
		if (ChainTaskTargetInfo.Num() == 0)
		{
			ChainTaskTargetInfo = TaskTargetInfo.ChainTaskTargetInfo;
		}
		if (TaskTargetParameter.Num() == 0)
		{
			TaskTargetParameter = TaskTargetInfo.TaskTargetParameter;
		}
	}

	//任务目标是否完成
	bool TaskTargetIsComplete(){ return TaskTargetCurCount >= TaskTargetMaxCount; }

	//该任务目标是否是一个必做任务目标
	bool TaskTargetIsMustDo() { return TaskTargetTag.HasTagExact(FGameplayTag::RequestGameplayTag("Task.Target.MustDo")); }

	//这次进度增加任务目前是否完成 目前增加进度
	bool AddProgress(int32 AddProgress, FName RoleSign = "None")
	{
		TaskTargetCurCount += AddProgress;
		bool TargetIsComplete = TaskTargetCurCount >= TaskTargetMaxCount;
		if (TargetIsComplete)
		{
			bTaskTargetIsEnd = true;
		}
		if (!RoleSign.IsNone())
		{
			RoleContribute.AddContribute(RoleSign, AddProgress);
		}
		return TargetIsComplete;
	}

	//获取任务目标参数
	bool GetParameterValue(FName ParameterName, float& Value)
	{
		for (FTaskParameter& Parameter : TaskTargetParameter)
		{
			if (Parameter.Name == ParameterName)
			{
				Value = Parameter.Value;
				return true;
			}
		}
		return false;
	}

public:
	/*任务目标的信息提供是自定义还是读表
	* 该值为true时，可以填写全部任务目标的内容
	* 该值为fasle时，必须填写一个有效的ID，UTS_Task会通过ID查找表进行内容初始化。 同时部分参数仍然可以重载
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCustom = false;

	/*任务目标ID 在任务目标ID之间唯一，不可重复
	* 如果填写了ID，在UTS_Task的StartTask中会读取数据表初始化任务属性，将除了TaskTargetText的属性进行刷新
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditConditionHides, EditCondition = "!bIsCustom"))
	int32 TaskTargetID;

	/*任务目标的时长（秒）该值为0.0f时表示无限时长
	* -1为该值的初始值，表示无效。该值有效且bIsCustom为false时，可以覆盖表数据的参数
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TaskTargetTime = -1.0f;
	//目标在时间结束时是否视为完成
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTimeEndComplete = false;

	/*任务目标的描述
	该值有效且bIsCustom为false时，可以覆盖表数据的参数
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText TaskTargetText;

	/*任务目标Tag  目标类型(是否是必做任务);目标所在地
	* 该值有效且bIsCustom为false时，可以覆盖表数据的参数
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Task.Target"))
	FGameplayTagContainer TaskTargetTag;

	//任务目标是否结束 完成了/时间到了
	UPROPERTY(BlueprintReadWrite)
	bool bTaskTargetIsEnd = false;

	//当前任务目标计数
	UPROPERTY(BlueprintReadWrite)
	int32 TaskTargetCurCount = 0;
	/*完成任务目标所需计数
	* -1为该值的初始值，表示无效。该值有效且bIsCustom为false时，可以覆盖表数据的参数
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TaskTargetMaxCount = -1;

	//任务目标贡献-不同角色完成的进度
	UPROPERTY(BlueprintReadWrite)
	FTaskRoleContribute RoleContribute;

	//任务目标的对照结构信息是否需要覆盖 同时在目标本身上表示着是否开启除了ID之外的其他检测方式
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCompareInfoIsOverride = false;
	//任务目标对照结构信息
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditConditionHides, EditCondition = "bCompareInfoIsOverride"))
	FTaskCompareInfo TaskTargetCompareInfo;

	/*连锁目标信息 当该任务目标完成时，会自动触发尝试添加的任务/任务目标ID
	* 该值有效且bIsCustom为false时，可以覆盖表数据的参数
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTaskChainInfo> ChainTaskTargetInfo;

	/*任务相关参数
	* 任务目标和任务均配置了同名参数时，优先获取TaskOverrideParameter
	* 该值有效且bIsCustom为false时，可以覆盖表数据的参数
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTaskParameter> TaskTargetParameter;
};

/*任务目标的刷新信息
*/
USTRUCT(BlueprintType)
struct FRefreshTaskTargetInfo
{
	GENERATED_BODY()
public:
	/*刷新的目标ID
	* 在进行刷新任务目标时ID比对未通过，还会使用TaskTargetCompareInfo的信息进行二次比对
	* 无论哪个比对通过了都会给目标加成AddProgress的进度
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RefreshTaskTargetID = -1;
	//任务目标对照结构信息
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTaskCompareInfo TaskTargetCompareInfo;
	//增加的进度
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AddProgress = 1;
	//导致本次增加进度的角色签名
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RoleSign;
};

/*单个任务奖励
*/
USTRUCT(BlueprintType)
struct FTaskReward
{
	GENERATED_BODY()
public:
	//奖励ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RewardID;
	//数量
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RewardNum = 1;
};

/*任务奖励
*/
USTRUCT(BlueprintType)
struct FTaskRewardInfo : public FTableRowBase
{
	GENERATED_BODY()
public:
	//任务奖励ID 任务奖励ID和任务奖励ID之间唯一，不可重复
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TaskRewardID;
	//任务奖励可以获得的个数 该值为负数时表示全部任务奖励均可获得
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TaskRewardNum = -1;

	//任务奖励<奖励ID，数量>
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FTaskReward> TaskReward;
};

/*任务结构体信息
*/
USTRUCT(BlueprintType)
struct FTaskInfo : public FTableRowBase
{
	GENERATED_BODY()
public:
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override
	{
		FTaskInfo* TaskInfo = InDataTable->FindRow<FTaskInfo>(InRowName, TEXT(""));
		if (TaskInfo && InRowName.ToString().IsNumeric())
		{
			TaskInfo->TaskID = FCString::Atoi(*InRowName.ToString());
		}
	}

	//获取任务参数
	bool GetParameterValue(FName ParameterName, float& Value)
	{
		for (FTaskParameter& Parameter : TaskOverrideParameter)
		{
			if (Parameter.Name == ParameterName)
			{
				Value = Parameter.Value;
				return true;
			}
		}
		//在任务目标里面寻找
		for (FTaskTargetInfo& TaskTarget : TaskTargetInfo)
		{
			if (TaskTarget.GetParameterValue(ParameterName, Value))
			{
				return true;
			}
		}
		return false;
	}

public:
	//任务ID 在任务ID之间唯一，不可重复
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TaskID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<class UTS_Task> TaskClass = UTS_TaskConfig::GetInstance()->DefaultTaskClass;

	//任务Tag  任务类型;任务所在地
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Task"))
	FGameplayTagContainer TaskTag;

	//任务名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText TaskName;
	//任务描述
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText TaskDescribe;

	//任务的时长（秒）该值为负数时表示无限时长
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TaskTime = -1.0f;

	//该任务是否有前置条件
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool TaskIsHaveCondition = false;

	/*任务前置条件（单个/多个）
	* 结论：如果Key（前置条件ID）在前置条件表中查询到了对应ID内容，使用前置条件表中的内容作为前置条件，否则使用使用Value（前置条件信息）的内容作为前置条件
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditConditionHides, EditCondition = "TaskIsHaveCondition"))
	TArray<FTaskConditionInfo> TaskConditionInfo;

	/*任务目标（单个/多个）完成全部目标，完成任意目标，组合情况 完成任务的条件
	* 结论：如果Key（目标ID）在目标表中查询到了对应ID内容，使用目标表中的内容作为目标，否则使用使用Value（目标信息）的内容作为目标
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTaskTargetInfo> TaskTargetInfo;

	/*完成任务所需完成的任务目标数量——该项适用于该任务没有必做目标的情况
	* 当TaskTargetInfo中一个必做任务目标都没有的时候启动该值进行判断完成的非必做目标数量
	* 当完成非必做目标数量 >= TaskCompleteTargetNum 时，任务也视为完成
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TaskCompleteTargetNum = 1;

	/*奖励<奖励ID，具体奖励内容>
	* 如果Key（奖励ID）为负数，使用Value（奖励信息）的内容作为奖励
	* 如果Key（奖励ID）为正数（某个奖励ID），将使用奖励表中配置对应的奖励信息作为奖励
	* 结论：如果Key（奖励ID）在奖励表中查询到了对应ID内容，使用奖励表中的内容作为奖励，否则使用使用Value（奖励信息）的内容作为奖励
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTaskRewardInfo> TaskRewardInfo;

	/*任务相关参数
	* 任务目标和任务均配置了同名参数时，优先获取TaskOverrideParameter
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTaskParameter> TaskOverrideParameter;
};