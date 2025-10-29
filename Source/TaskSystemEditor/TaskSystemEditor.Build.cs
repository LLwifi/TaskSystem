// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TaskSystemEditor : ModuleRules
{
	public TaskSystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core", "UMGEditor", "Blutility","UnrealEd",
                "CommonCompare",//该模块依赖另外一个插件
                // ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG",
                "DeveloperSettings",
                "ScriptableEditorWidgets",  //	UE5+以上的版本需要该模块
                "TaskSystem",
                "InputCore",//SComboBox需要
				"ToolWidgets",//SSearchableComboBox需要
                "PropertyEditor",//SPropertyEditorCombo需要
				"GameplayTags",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
