/* EngineSubscriber.cpp
 * Copyright 2011 Brian Hill
 * All rights reserved. Distributed under the terms of the BSD License.
 */
#include "EngineSubscriber.h"


SubscriberHandler::SubscriberHandler(EngineSubscriber *owner)
	:
	BHandler("ESM_Handler")
{
	fOwner = owner;
}

void
SubscriberHandler::MessageReceived(BMessage *message)
{
	printf("ESM_Handler received message\n");
	fOwner->_ProcessEngineMessage(message);
}


EngineSubscriber::EngineSubscriber()
{
	fUniqueID = time(NULL);

	//Start the Looper running
	fHandler = new SubscriberHandler(this);
	fLooper = new BLooper("ESM_Looper");
	fLooper->AddHandler(fHandler);
	fLooper->Run();
}


EngineSubscriber::~EngineSubscriber()
{
	fLooper->Lock();
	fLooper->Quit();
}


void
EngineSubscriber::_SubscribeToEngine(int itemCount, int numberOfLaunchesScale, int firstLaunchScale,
							int latestLaunchScale, int lastIntervalScale, int totalRuntimeScale,
							BMessage *excludeList = NULL)
{
	// Subscribe with the Einsteinium Engine to receive updates
	BMessage subscribeMsg(E_SUBSCRIBE_RANKED_APPS);
	subscribeMsg.AddInt32(E_SUBSCRIPTION_UNIQUEID, fUniqueID);
	subscribeMsg.AddInt16(E_SUBSCRIPTION_COUNT, itemCount);
	subscribeMsg.AddInt8(E_SUBSCRIPTION_LAUNCH_SCALE, numberOfLaunchesScale);
	subscribeMsg.AddInt8(E_SUBSCRIPTION_FIRST_SCALE, firstLaunchScale);
	subscribeMsg.AddInt8(E_SUBSCRIPTION_LAST_SCALE, latestLaunchScale);
	subscribeMsg.AddInt8(E_SUBSCRIPTION_INTERVAL_SCALE, lastIntervalScale);
	subscribeMsg.AddInt8(E_SUBSCRIPTION_RUNTIME_SCALE, totalRuntimeScale);
	if(excludeList)
	{
		type_code typeFound;
		int32 countFound;
		status_t exStatus = excludeList->GetInfo(E_SUBSCRIPTION_EXCLUSIONS, &typeFound, &countFound);
		BString appSig;
		if(exStatus==B_OK)
		{
			for(int i=0; i<countFound; i++)
			{
				excludeList->FindString(E_SUBSCRIPTION_EXCLUSIONS, i, &appSig);
				subscribeMsg.AddString(E_SUBSCRIPTION_EXCLUSIONS, appSig);
			}
		}
	}
	status_t mErr;
	BMessenger messenger(fHandler, NULL, &mErr);
	if(!messenger.IsValid())
	{
		printf("ESM: Messenger is not valid, error=%s\n", strerror(mErr));
		return;
	}
	subscribeMsg.AddMessenger(E_SUBSCRIPTION_MESSENGER, messenger);
	BMessenger EsMessenger(einsteinium_engine_sig);
	EsMessenger.SendMessage(&subscribeMsg, fHandler);
	// TODO trying to get reply synchronously freezes
	//	BMessage reply;
	//	EsMessenger.SendMessage(&subscribeMsg, &reply);
}


void
EngineSubscriber::_UnsubscribeFromEngine()
{
	// Unsubscribe from the Einsteinium Engine
	if (be_roster->IsRunning(einsteinium_engine_sig))
	{
		BMessage unsubscribeMsg(E_UNSUBSCRIBE_RANKED_APPS);
		unsubscribeMsg.AddInt32(E_SUBSCRIPTION_UNIQUEID, fUniqueID);
		BMessenger EsMessenger(einsteinium_engine_sig);
		EsMessenger.SendMessage(&unsubscribeMsg, fHandler);
	}
}


bool
EngineSubscriber::_IsEngineRunning()
{
	return be_roster->IsRunning(einsteinium_engine_sig);
}


status_t
EngineSubscriber::_LaunchEngine()
{
	return be_roster->Launch(einsteinium_engine_sig);
}


status_t
EngineSubscriber::_QuitEngine()
{
	status_t rc = B_ERROR;
	BMessenger appMessenger(einsteinium_engine_sig, -1, &rc);
	if(!appMessenger.IsValid())
		return rc;
	rc = appMessenger.SendMessage(B_QUIT_REQUESTED);
	return rc;
}


void
EngineSubscriber::_ProcessEngineMessage(BMessage *message)
{
	switch(message->what)
	{
		case E_SUBSCRIBE_FAILED: {
			printf("ESM_Handler received \'Subscribe Failed\' message\n");
			_SubscribeFailed();
			break;
		}
		case E_SUBSCRIBE_CONFIRMED: {
			printf("ESM_Handler received \'Subscribe Confirmed\' message\n");
			_SubscribeConfirmed();
			break;
		}
		case E_SUBSCRIBER_UPDATE_RANKED_APPS: {
			printf("ESM_Handler received \'Update Ranked Apps\' message\n");
			_UpdateReceived(message);
			break;
		}
	}
}

