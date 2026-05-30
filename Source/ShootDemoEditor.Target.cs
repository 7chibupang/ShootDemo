// ShootDemoEditor.Target.cs — 编辑器构建目标

using UnrealBuildTool;
using System.Collections.Generic;

public class ShootDemoEditorTarget : TargetRules
{
	public ShootDemoEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V3;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("ShootDemo");
	}
}
