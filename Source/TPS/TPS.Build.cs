// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPS : ModuleRules
{
	public TPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore",
                                                            "HeadMountedDisplay", "NavigationSystem",
                                                            "AIModule", "PhysicsCore", "Slate" });
    }
}
