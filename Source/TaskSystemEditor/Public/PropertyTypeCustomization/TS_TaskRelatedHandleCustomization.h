// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <TS_StructAndEnum.h>
#include "IPropertyTypeCustomization.h"


//class SWidget;

/**
 * 
 */
class TASKSYSTEMEDITOR_API ITS_TaskRelatedHandleCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance()
    {
        return MakeShareable(new ITS_TaskRelatedHandleCustomization());
    }

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

    //刷新
    void Refresh(TArray<TSharedPtr<FString>>& AllRowName);

    void OnSelectionChanged_Type(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectionInfo);
    TSharedRef<SWidget> OnGenerateWidget_Type(TSharedPtr<FString> InItem);

    //改变选择的回调
    void OnSelectionChanged_RowName(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectionInfo);
    //每个下拉选择框的样式
    TSharedRef<SWidget> OnGenerateWidget_RowName(TSharedPtr<FString> InItem);

    FString GetCurTaskRelatedNameFromHandle();

public:
    //任务相关类型
    TMap<FString, ETS_TaskRelatedType> Name_Type;
    TSharedPtr<IPropertyHandle> TypeHandle;
    TArray<TSharedPtr<FString>> TypesNames;
    TSharedPtr<class SSearchableComboBox> SearchableComboBox_Type;
    TSharedPtr<class STextBlock> ComboBox_Type_Text;

    //行名称
    TSharedPtr<IPropertyHandle> RowNameHandle;
    TArray<TSharedPtr<FString>> RowNames;
    TSharedPtr<class SSearchableComboBox> SearchableComboBox_RowName;
    TSharedPtr<class STextBlock> ComboBox_Name_Text;

    //行名称和对应结构（主要是显示名称）对应的Map
    TMap<FName, FTaskTargetInfo> TaskTarget_RowName_Info;
    TMap<FName, FTaskInfo> Task_RowName_Info;
    //显示名称和对应结构（主要是显示名称）对应的Map
    TMap<FString, FTaskTargetInfo> TaskTarget_DisplayName_Info;
    TMap<FString, FTaskInfo> Task_DisplayName_Info;

    //反射出来的结构体数据
    FTS_TaskRelatedHandle* TaskRelatedHandle;


};
