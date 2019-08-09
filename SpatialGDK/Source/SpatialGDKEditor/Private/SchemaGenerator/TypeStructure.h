// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Net/RepLayout.h"

/*

This file contains functions to generate an abstract syntax tree which is used by the code generating in
SchemaGenerator.cpp. The AST follows a structure which is similar to a UClass/UProperty
structure, but also contains additional metadata such as replication data. One main difference is that the AST
structure will recurse into object properties if it's determined that the container type holds a strong reference
to that subobject (such as a character owning its movement component).

The AST is built by recursing through the UClass/UProperty structure generated by UHT. Afterwards, the rep layout
is generated with FRepLayout::InitFromObjectClass, and the replication handle / etc is stored in FUnrealRepData
and transposed into the correct part of the AST.

An example AST generated from ACharacter:

FUnrealType
	+ Type: Character
	+ Properties:
		[0] FUnrealProperty
			+ Property: "MovementComp"
			+ Type: FUnrealType
				+ Type: CharacterMovementComponent
				+ Properties:
					[0] ....
					...
				+ RPCs:
					[0] FUnrealRPC
						+ CallerType: CharacterMovementComponent
						+ Function: "ServerMove"
						+ Type: RPC_Server
						+ bReliable: false
						+ Parameters:
							[0] FUnrealProperty
							...
					[1] ....
			+ ReplicationData: FUnrealRepData
				+ RepLayoutType: REPCMD_PropertyObject
				+ Handle: 29
				+ ...
		[1] FUnrealProperty
			+ Property: "bIsCrouched"
			+ Type: nullptr
			+ ReplicationData: FUnrealRepData
				+ RepLayoutType: REPCMD_PropertyBool
				+ Handle: 15
				...
		[2] FUnrealProperty
			+ Property: "Controller":
			+ Type: nullptr						<- weak reference so not set.
			+ ReplicationData: FUnrealRepData
				+ RepLayoutType: REPCMD_PropertyObject
				+ Handle: 19
				...
		[3] FUnrealProperty
			+ Property: "SomeTransientProperty"
			+ Type: nullptr
			+ ReplicationData: nullptr
			+ HandoverData: FUnrealHandoverData
				+ RepLayoutType: REPCMD_PropertyFloat
				+ Handle: 1
				...
	+ RPCs:
		[0] FUnrealRPC
			+ CallerType: Character
			+ Function: ClientRestart
			+ Type: RPC_Client
			+ bReliable: true
		[1] ....
*/

// As we cannot fully implement replication conditions using SpatialOS's component interest API, we instead try
// to emulate it by separating all replicated properties into two groups: properties which are meant for just one
// client (AutonomousProxy/OwnerOnly), or many clients (everything else).
enum EReplicatedPropertyGroup
{
	REP_SingleClient,
	REP_MultiClient
};

// RPC Type. This matches the tag specified in UPROPERTY. For example, RPC_Client means RPCs which are destined for
// the client (such as ClientAckGoodMove etc).
enum ERPCType
{
	RPC_Client,
	RPC_Server,
	RPC_CrossServer,
	RPC_NetMulticast,
	RPC_Unknown
};

struct FUnrealProperty;
struct FUnrealRPC;
struct FUnrealRepData;
struct FUnrealHandoverData;

// A node which represents an unreal type, such as ACharacter or UCharacterMovementComponent.
struct FUnrealType
{
	FString ObjectPath; // Path for the object. Either a CDO etc etc blah blah.
	bool bObjectEditorOnly;
	FName Name; // Name for the object. This is either the name of the object itself, or the name of the property in the blueprint
	TMultiMap<UProperty*, TSharedPtr<FUnrealProperty>> PropertiesMap; // Only needed at generation time.
	TArray<TSharedPtr<FUnrealProperty>> PropertiesList;
	uint32 NumRPCs;
	TWeakPtr<FUnrealProperty> ParentProperty;
	bool bIsActorClass;
	bool bIsActorComponent;
	FString ClassPath;
	FString ClassName;
};

// A node which represents a single property or parameter in an RPC.
struct FUnrealProperty
{
	//UProperty* Property;
	TSharedPtr<FUnrealType> Type; // Only set if strong reference to object/struct property.
	TSharedPtr<FUnrealRepData> ReplicationData; // Only set if property is replicated.
	TSharedPtr<FUnrealHandoverData> HandoverData; // Only set if property is marked for handover (and not replicated).
	TWeakPtr<FUnrealType> ContainerType; // Not set if this property is an RPC parameter.

	// These variables are used for unique variable checksum generation. We do this to accurately match properties at run-time.
	// They are used in the function GenerateChecksum which will use all three variables and the UProperty itself to create a checksum for each FUnrealProperty.
	int32 StaticArrayIndex;
	uint32 CompatibleChecksum;
	uint32 ParentChecksum;

	bool bObjectProperty;
	bool bStructProperty;
	bool bArrayProperty;
	bool bObjectArrayProperty;

	FString PropertyPath;
	FString PropertyName;

	int32 ArrayDim;
	EPropertyFlags PropertyFlags;
	EStructFlags StructFlags;

	FString DataType;
};

// A node which represents an RPC.
struct FUnrealRPC
{
	UClass* CallerType;
	UFunction* Function;
	ERPCType Type;
	TMap<UProperty*, TSharedPtr<FUnrealProperty>> Parameters;
	bool bReliable;
};

// A node which represents replication data generated by the FRepLayout instantiated from a UClass.
struct FUnrealRepData
{
	ERepLayoutCmdType RepLayoutType;
	ELifetimeCondition Condition;
	ELifetimeRepNotifyCondition RepNotifyCondition;
	uint16 Handle;
	int32 RoleSwapHandle;
	int32 ArrayIndex;
};

// A node which represents handover (server to server) data.
struct FUnrealHandoverData
{
	uint16 Handle;
};

using FUnrealFlatRepData = TMap<EReplicatedPropertyGroup, TMap<uint16, TSharedPtr<FUnrealProperty>>>;
using FUnrealRPCsByType = TMap<ERPCType, TArray<TSharedPtr<FUnrealRPC>>>;
using FCmdHandlePropertyMap = TMap<uint16, TSharedPtr<FUnrealProperty>>;
using FSubobjectMap = TMap<uint32, TSharedPtr<FUnrealType>>;

// Given a UClass, returns either "AFoo" or "UFoo" depending on whether Foo is a subclass of actor.
FString GetFullCPPName(UClass* Class);

// Utility functions to convert certain enum values to strings.
FString GetLifetimeConditionAsString(ELifetimeCondition Condition);
FString GetRepNotifyLifetimeConditionAsString(ELifetimeRepNotifyCondition Condition);

// Get an array of all replicated property groups
TArray<EReplicatedPropertyGroup> GetAllReplicatedPropertyGroups();

// Convert a replicated property group to a string. Used to generate component names.
FString GetReplicatedPropertyGroupName(EReplicatedPropertyGroup Group);

// Get an array of all RPC types.
TArray<ERPCType> GetRPCTypes();

// Given a UFunction, determines the RPC type.
ERPCType GetRPCTypeFromFunction(UFunction* Function);

// Converts an RPC type to string. Used to generate component names.
FString GetRPCTypeName(ERPCType RPCType);

// Similar to 'VisitAllObjects', but instead applies the Visitor function to all properties which are traversed.
void VisitAllPropertiesMap(TSharedPtr<FUnrealType> TypeNode, TFunction<bool(TSharedPtr<FUnrealProperty>)> Visitor, bool bRecurseIntoSubobjects);

void VisitAllPropertiesList(TSharedPtr<FUnrealType> TypeNode, TFunction<bool(TSharedPtr<FUnrealProperty>)> Visitor, bool bRecurseIntoSubobjects);

// Similar to 'VisitAllObjects', but instead applies the Visitor function to all parameters in an RPC (and subproperties of structs/objects where appropriate).
//void VisitAllProperties(TSharedPtr<FUnrealRPC> RPCNode, TFunction<bool(TSharedPtr<FUnrealProperty>)> Visitor, bool bRecurseIntoSubobjects);

// Generates a unique checksum for the Property that allows matching to Unreal's RepLayout Cmds.
uint32 GenerateChecksum(UProperty* Property, uint32 ParentChecksum, int32 StaticArrayIndex);

// Creates a new FUnrealProperty for the included UProperty, generates a checksum for it and then adds it to the TypeNode included.
TSharedPtr<FUnrealProperty> CreateUnrealProperty(TSharedPtr<FUnrealType> TypeNode, UProperty* Property, uint32 ParentChecksum, uint32 StaticArrayIndex);

// Generates an AST from an Unreal UStruct or UClass.
TSharedPtr<FUnrealType> CreateUnrealTypeInfo(UStruct* Type, uint32 ParentChecksum, int32 StaticArrayIndex, bool bIsRPC);

// Traverses an AST, and generates a flattened list of replicated properties, which will match the Cmds array of FRepLayout.
// The list of replicated properties will all have the ReplicatedData field set to a valid FUnrealRepData node which contains
// data such as the handle or replication condition.
//
// This function will _not_ traverse into subobject properties (as the replication system deals with each object separately).
FUnrealFlatRepData GetFlatRepData(TSharedPtr<FUnrealType> TypeInfo);

// Traverses an AST, and generates a flattened list of handover properties. The list of handover properties will all have
// the HandoverData field set to a value FUnrealHandoverData node which contains data such as the handle or replication type.
//
// This function will traverse into subobject properties.
FCmdHandlePropertyMap GetFlatHandoverData(TSharedPtr<FUnrealType> TypeInfo);

// Traverses an AST fully (including subobjects) and generates a list of all RPCs which would be routed through an actor channel
// of the Unreal class represented by TypeInfo.
//
// This function will traverse into subobject properties.
//FUnrealRPCsByType GetAllRPCsByType(TSharedPtr<FUnrealType> TypeInfo);

// Given a property, traverse up to the root property and create a list of properties needed to reach the leaf property.
// For example: foo->bar->baz becomes {"foo", "bar", "baz"}.
TArray<TSharedPtr<FUnrealProperty>> GetPropertyChain(TSharedPtr<FUnrealProperty> LeafProperty);

// Get all default subobjects for an actor.
FSubobjectMap GetAllSubobjects(TSharedPtr<FUnrealType> TypeInfo);

void CleanPropertyMaps(TSharedPtr<FUnrealType> TypeInfo);

FString PropertyToSchemaType(UProperty* Property);

// Return the string representation of the underlying data type of an enum property
FString GetEnumDataType(const UEnumProperty* EnumProperty);
