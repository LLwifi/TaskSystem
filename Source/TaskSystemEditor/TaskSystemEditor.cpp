// Copyright Epic Games, Inc. All Rights Reserved.

#include "TaskSystemEditor.h"
#include "PropertyTypeCustomization/TS_TaskRelatedHandleCustomization.h"


void FTaskSystemEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	// 加载 PropertyEditor 模块
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyEditorModule.RegisterCustomPropertyTypeLayout(FName("TS_TaskRelatedHandle"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&ITS_TaskRelatedHandleCustomization::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FTaskSystemEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("TS_TaskRelatedHandle");
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

IMPLEMENT_MODULE(FTaskSystemEditorModule, TaskSystemEditor)