using UnrealBuildTool;
public class KowloonFgame : ModuleRules
{
 public KowloonFgame(ReadOnlyTargetRules Target) : base(Target)
 {
  PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","InputCore","EnhancedInput","PCG","AIModule","NavigationSystem","Json","JsonUtilities","PhysicsCore","Slate","SlateCore"});
  if(Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new string[]{"UnrealEd","AssetRegistry","AudioEditor","AnimGraph","BlueprintGraph","KismetCompiler","AnimGraphRuntime"});
 }
}

