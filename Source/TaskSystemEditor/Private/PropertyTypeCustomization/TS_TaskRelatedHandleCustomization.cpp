// Fill out your copyright notice in the Description page of Project Settings.


#include "PropertyTypeCustomization/TS_TaskRelatedHandleCustomization.h"
#include <IDetailChildrenBuilder.h>
#include <Widgets/Input/SEditableTextBox.h>
#include <Misc/MessageDialog.h>
#include <Misc/PackageName.h>
#include <FileHelpers.h>
#include <DetailWidgetRow.h>
#include "SSearchableComboBox.h"
#include <PropertyCustomizationHelpers.h>
#include <Engine/CompositeDataTable.h>

#define LOCTEXT_NAMESPACE "TS_TaskRelatedHandleCustomization"

void ITS_TaskRelatedHandleCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    HeaderRow
        .NameContent()
        [
            PropertyHandle->CreatePropertyNameWidget()
        ];
}

void ITS_TaskRelatedHandleCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    TypeHandle = PropertyHandle->GetChildHandle("Type");
    RowNameHandle = PropertyHandle->GetChildHandle("ID");

    void* ValuePtr;
    PropertyHandle->GetValueData(ValuePtr);
    if (ValuePtr != nullptr)//在动画通知上同时选择两个时会为空
    {
        TaskRelatedHandle = (FTS_TaskRelatedHandle*)ValuePtr;

        Refresh(RowNames);

        //获取枚举的显示名称
        UEnum* EnumPtr = StaticEnum<ETS_TaskRelatedType>();
        if (EnumPtr)
        {
            for (int32 i = 0; i < EnumPtr->GetMaxEnumValue(); ++i)
            {
                FString DisplayName = EnumPtr->GetNameStringByValue(i);
                ETS_TaskRelatedType TaskRelatedType = (ETS_TaskRelatedType)EnumPtr->GetValueByIndex(i);
                Name_Type.Add(DisplayName, TaskRelatedType);
                TypesNames.Add(MakeShareable(new FString(DisplayName)));
            }
        }


        //slate
        ChildBuilder.AddCustomRow(FText())
            [
                SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    [
                        SAssignNew(SearchableComboBox_Type, SSearchableComboBox)
                            .OptionsSource(&TypesNames)//所有选项
                            .OnGenerateWidget(this, &ITS_TaskRelatedHandleCustomization::OnGenerateWidget_Type)//每个下拉选项的样式通过函数构造
                            .OnSelectionChanged(this, &ITS_TaskRelatedHandleCustomization::OnSelectionChanged_Type)//改变选择的回调
                            [
                                SAssignNew(ComboBox_Type_Text, STextBlock)
                                    .Text(FText::FromString(EnumPtr->GetNameStringByValue((int32)TaskRelatedHandle->Type)))
                            ]
                    ]
                    + SVerticalBox::Slot()
                    [
                        SAssignNew(SearchableComboBox_RowName, SSearchableComboBox)
                            .OptionsSource(&RowNames)//所有选项
                            .OnGenerateWidget(this, &ITS_TaskRelatedHandleCustomization::OnGenerateWidget_RowName)//每个下拉选项的样式通过函数构造
                            .OnSelectionChanged(this, &ITS_TaskRelatedHandleCustomization::OnSelectionChanged_RowName)//改变选择的回调
                            [
                                SAssignNew(ComboBox_Name_Text, STextBlock)
                                    .Text(FText::FromString(*GetCurTaskRelatedNameFromHandle()))
                            ]
                    ]
                    //+ SVerticalBox::Slot().AutoHeight()
                    //[
                    //    SNew(SHorizontalBox)
                    //        +SHorizontalBox::Slot().AutoWidth()
                    //        [
                    //            DataTableHandle->CreatePropertyValueWidget()
                    //        ]
                    //        + SHorizontalBox::Slot().AutoWidth()
                    //        [
                    //            SoundBaseHandle->CreatePropertyValueWidget()
                    //        ]
                    //        + SHorizontalBox::Slot().AutoWidth()
                    //        [
                    //            ReferenceObjectHandle->CreatePropertyValueWidget()
                    //        ]
                    //]
            ];
    }
}

void ITS_TaskRelatedHandleCustomization::Refresh(TArray<TSharedPtr<FString>>& AllRowName)
{
    AllRowName.Empty();

    //当前类型的表
    UDataTable* DataTable = nullptr;

    switch (TaskRelatedHandle->Type)
    {
    case ETS_TaskRelatedType::TaskTarget:
    {
        DataTable = UTS_TaskConfig::GetInstance()->AllTaskTarget.LoadSynchronous();
        if (DataTable)
        {
            for (FName& RowName : DataTable->GetRowNames())
            {
                FTaskTargetInfo* TaskTargetInfo = DataTable->FindRow<FTaskTargetInfo>(RowName, TEXT(""));
                if (TaskTargetInfo)
                {
                    FString Left = RowName.ToString() + "|";
                    TaskTarget_RowName_Info.Add(RowName, *TaskTargetInfo);
                    TaskTarget_DisplayName_Info.Add(Left + TaskTargetInfo->TaskTargetText.ToString(), *TaskTargetInfo);
                    AllRowName.Add(MakeShareable(new FString(Left + TaskTargetInfo->TaskTargetText.ToString())));
                }
            }
        }
        break;
    }
    case ETS_TaskRelatedType::Task:
    {
        DataTable = UTS_TaskConfig::GetInstance()->AllTask.LoadSynchronous();
        if (DataTable)
        {
            for (FName& RowName : DataTable->GetRowNames())
            {
                FTaskInfo* TaskInfo = DataTable->FindRow<FTaskInfo>(RowName, TEXT(""));
                if (TaskInfo)
                {
                    FString Left = RowName.ToString() + "|";
                    Task_RowName_Info.Add(RowName, *TaskInfo);
                    Task_DisplayName_Info.Add(Left + TaskInfo->TaskName.ToString(), *TaskInfo);
                    AllRowName.Add(MakeShareable(new FString(Left + TaskInfo->TaskName.ToString())));
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

void ITS_TaskRelatedHandleCustomization::OnSelectionChanged_Type(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectionInfo)
{
    ComboBox_Type_Text->SetText(FText::FromString(*InItem));//下拉框的文本改变
    TaskRelatedHandle->Type = Name_Type[*InItem.Get()];

    Refresh(RowNames);
    SearchableComboBox_RowName->RefreshOptions();
}

TSharedRef<SWidget> ITS_TaskRelatedHandleCustomization::OnGenerateWidget_Type(TSharedPtr<FString> InItem)
{
    return SNew(STextBlock)
        .Text(FText::FromString(*InItem));
}

void ITS_TaskRelatedHandleCustomization::OnSelectionChanged_RowName(TSharedPtr<FString> InItem, ESelectInfo::Type InSelectionInfo)
{
    RowNameHandle->SetValue(FName(*InItem.Get()));
    ComboBox_Name_Text->SetText(FText::FromString(*InItem));

    TypeHandle->NotifyPreChange();

    switch (TaskRelatedHandle->Type)
    {
    case ETS_TaskRelatedType::TaskTarget:
    {
        if (TaskTarget_DisplayName_Info.Contains(*InItem.Get()))
        {
            TaskRelatedHandle->ID = TaskTarget_DisplayName_Info[*InItem.Get()].TaskTargetID;
        }
        break;
    }
    case ETS_TaskRelatedType::Task:
    {
        if (Task_DisplayName_Info.Contains(*InItem.Get()))
        {
            TaskRelatedHandle->ID = Task_DisplayName_Info[*InItem.Get()].TaskID;
        }
        break;
    }
    default:
        break;
    }

    TypeHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

TSharedRef<SWidget> ITS_TaskRelatedHandleCustomization::OnGenerateWidget_RowName(TSharedPtr<FString> InItem)
{
    return SNew(STextBlock)
        .Text(FText::FromString(*InItem));
}

FString ITS_TaskRelatedHandleCustomization::GetCurTaskRelatedNameFromHandle()
{
    FString Left, Right;
    switch (TaskRelatedHandle->Type)
    {
    case ETS_TaskRelatedType::TaskTarget:
    {
        if (TaskTarget_RowName_Info.Contains(FName(*FString::FromInt(TaskRelatedHandle->ID))))
        {  
            Left = FString::FromInt(TaskRelatedHandle->ID) + "|";
            Right = TaskTarget_RowName_Info[FName(*FString::FromInt(TaskRelatedHandle->ID))].TaskTargetText.ToString();
        }
        break;
    }
    case ETS_TaskRelatedType::Task:
    {
        if (Task_RowName_Info.Contains(FName(*FString::FromInt(TaskRelatedHandle->ID))))
        {
            Left = FString::FromInt(TaskRelatedHandle->ID) + "|";
            Right = Task_RowName_Info[FName(*FString::FromInt(TaskRelatedHandle->ID))].TaskName.ToString();
        }
        break;
    }
    default:
        break;
    }

    return Left + Right;
}

#undef LOCTEXT_NAMESPACE