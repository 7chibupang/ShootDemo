// ShootDemo.h — 模块主头文件

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FShootDemoModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
