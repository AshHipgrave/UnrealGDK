// Copyright (c) Improbable Worlds Ltd, All Rights Reserved
#include "SpatialGDKEditorCloudLauncherSettings.h"

#include "SpatialGDKEditorSettings.h";
#include "Settings/LevelEditorPlaySettings.h"
#include "Internationalization/Regex.h"
#include "Templates/SharedPointer.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"

USpatialGDKEditorCloudLauncherSettings::USpatialGDKEditorCloudLauncherSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	const USpatialGDKEditorSettings * SpatialEditorSettings = GetDefault<USpatialGDKEditorSettings>();
	SnapshotPath.FilePath = FPaths::Combine(SpatialEditorSettings->GetSpatialOSSnapshotFolderPath(), SpatialEditorSettings->GetSpatialOSSnapshotFile());
	ProjectName = GetProjectNameFromSpatial();
}

FString USpatialGDKEditorCloudLauncherSettings::ToString()
{
	TArray<FStringFormatArg> Args;

	Args.Add(ProjectName);
	Args.Add(PrimaryDeploymentName);
	Args.Add(AssemblyName);
	Args.Add(PrimaryLaunchConfigPath.FilePath);
	Args.Add(SnapshotPath.FilePath);
	Args.Add(SimulatedPlayerDeploymentName);
	Args.Add(SimulatedPlayerLaunchConfigPath.FilePath);
	Args.Add(NumberOfSimulatedPlayers);
	Args.Add(PrimaryDeploymentNameIsValid);
	Args.Add(AssemblyNameIsValid);
	Args.Add(ProjectNameIsValid);
	Args.Add(SimulatedPlayersIsEnabled);

	return FString::Format(TEXT(
		"ProjectName={0}, "
		"PrimaryDeploymentName={1}"
		"AssemblyName={2}"
		"PrimaryLaunchConfigPath={3}, "
		"SnapshotPath={4}, "
		"SimulatedPlayerDeploymentName={5}, "
		"SimulatedPlayerLaunchConfigPath={6}, "
		"NumberOfSimulatedPlayers={7}, "
		"PrimaryDeploymentNameIsValid={8}, "
		"AssemblyNameIsValid={9}, "
		"ProjectNameIsValid={10}, "
		"SimulatedPlayersIsEnabled={11}")
		, Args);
}

FString USpatialGDKEditorCloudLauncherSettings::GetProjectNameFromSpatial()
{
	FString FileContents;
	FString SpatialOSFile = GetDefault<USpatialGDKEditorSettings>()->GetSpatialOSDirectory().Append(TEXT("/spatialos.json"));

	if (!FFileHelper::LoadFileToString(FileContents, *SpatialOSFile))
	{
		return TEXT("");
	}
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
	TSharedPtr<FJsonObject> JsonObject;

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		return JsonObject->GetStringField("name");
	}
	return TEXT("");
}

void USpatialGDKEditorCloudLauncherSettings::ValidateAssemblyName()
{
	const FRegexPattern AssemblyPattern(TEXT("^[a-zA-Z0-9_.-]{5,64}$"));
	FRegexMatcher RegMatcher(AssemblyPattern, AssemblyName);

	AssemblyNameIsValid = RegMatcher.FindNext();
}

void USpatialGDKEditorCloudLauncherSettings::ValidateProjectName()
{
	const FRegexPattern ProjectPattern(TEXT("^[a-z0-9_]{3,32}$"));
	FRegexMatcher RegMatcher(ProjectPattern, ProjectName);

	ProjectNameIsValid = RegMatcher.FindNext();
}

void USpatialGDKEditorCloudLauncherSettings::ValidateDeploymentName()
{
	const FRegexPattern DeploymentPattern(TEXT("^[a-z0-9_]{2,32}$"));
	FRegexMatcher RegMatcher(DeploymentPattern, PrimaryDeploymentName);

	PrimaryDeploymentNameIsValid = RegMatcher.FindNext();
}

void USpatialGDKEditorCloudLauncherSettings::SetPrimaryDeploymentName(const FString & Name)
{
	PrimaryDeploymentName = Name;
}

void USpatialGDKEditorCloudLauncherSettings::SetAssemblyName(const FString & Name)
{
	AssemblyName = Name;
}

void USpatialGDKEditorCloudLauncherSettings::SetProjectName(const FString & Name)
{
	ProjectName = Name;
}

void USpatialGDKEditorCloudLauncherSettings::SetPrimaryLaunchConfigPath(const FString & Path)
{
	PrimaryLaunchConfigPath.FilePath = FPaths::ConvertRelativePathToFull(Path);
}

void USpatialGDKEditorCloudLauncherSettings::SetSnapshotPath(const FString & Path)
{
	SnapshotPath.FilePath = FPaths::ConvertRelativePathToFull(Path);
}

void USpatialGDKEditorCloudLauncherSettings::SetSimulatedPlayersEnabledState(bool IsEnabled)
{
	SimulatedPlayersIsEnabled = IsEnabled;
}

void USpatialGDKEditorCloudLauncherSettings::SetSimulatedPlayerDeploymentName(const FString & Name)
{
	SimulatedPlayerDeploymentName = Name;
}

void USpatialGDKEditorCloudLauncherSettings::SetSimulatedPlayerLaunchConfigPath(const FString & Path)
{
	SimulatedPlayerLaunchConfigPath.FilePath = FPaths::ConvertRelativePathToFull(Path);
}

void USpatialGDKEditorCloudLauncherSettings::SetNumberOfSimulatedPlayers(uint32 Number)
{
	NumberOfSimulatedPlayers = Number;
}
