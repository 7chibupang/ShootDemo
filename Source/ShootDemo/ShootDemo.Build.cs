// ShootDemo.Build.cs — 模块构建配置

using UnrealBuildTool;

public class ShootDemo : ModuleRules
{
	public ShootDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"UMG",
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});

		// 如果启用头戴显示设备（VR），取消注释
		// PrivateDependencyModuleNames.Add("HeadMountedDisplay");
	}
}
