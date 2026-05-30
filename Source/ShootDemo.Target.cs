// ShootDemo.Target.cs — 游戏构建目标

using UnrealBuildTool;
using System.Collections.Generic;

public class ShootDemoTarget : TargetRules
{
	public ShootDemoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V3;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("ShootDemo");
	}
}
