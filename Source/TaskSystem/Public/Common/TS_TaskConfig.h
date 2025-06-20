// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include <Engine/DataTable.h>
#include "TS_TaskConfig.generated.h"

/**
 * 编辑器下的通用Task配置
 */
UCLASS(config = TS_TaskConfig, defaultconfig)
class TASKSYSTEM_API UTS_TaskConfig : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const final override
	{
		return FName("GameEditorConfig");
	}
	static UTS_TaskConfig* GetInstance(){ return GetMutableDefault<UTS_TaskConfig>(); }
	UFUNCTION(BlueprintPure, BlueprintCallable)
		static UTS_TaskConfig* GetTS_TaskConfig() { return GetInstance(); }

public:
	//全部任务表
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UDataTable> AllTask;

	//全部任务目标表
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UDataTable> AllTaskTarget;

	//默认的任务类
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<class UTS_Task> DefaultTaskClass;
};
