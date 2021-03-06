// Copyright 2015-2020 Mail.Ru Group. All Rights Reserved.

#include "PsDataEvent.h"

#include "PsData.h"
#include "PsDataField.h"

#include "Async/Async.h"

const FString UPsDataEvent::Added(TEXT("Added"));
const FString UPsDataEvent::Removing(TEXT("Removing"));
const FString UPsDataEvent::Changed(TEXT("Changed"));
const FString UPsDataEvent::NameChanged(TEXT("NameChanged"));

UPsDataEvent::UPsDataEvent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Type(TEXT("Unknown"))
	, Target(nullptr)
	, bBubbles(false)
	, bStopImmediate(false)
	, bStop(false)
{
}

UPsDataEvent* UPsDataEvent::ConstructEvent(FString EventType, bool bEventBubbles)
{
	UPsDataEvent* Event = NewObject<UPsDataEvent>();

	Event->Type = EventType;
	Event->bBubbles = bEventBubbles;

	return Event;
}

const UPsData* UPsDataEvent::GetTarget() const
{
	return Target;
}

const FString& UPsDataEvent::GetType() const
{
	return Type;
}

bool UPsDataEvent::IsBubbles() const
{
	return bBubbles;
}

void UPsDataEvent::StopImmediatePropagation()
{
	bStopImmediate = true;
	bStop = true;
}

void UPsDataEvent::StopPropagation()
{
	bStop = true;
}

UPsData* UPsDataEvent::GetTarget_Mutable() const
{
	return Target;
}
