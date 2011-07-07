//
// Licensed to Green Energy Corp (www.greenenergycorp.com) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  Green Enery Corp licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
#include "AsyncPhysLayerMonitor.h"

#include "IPhysicalLayerAsync.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <assert.h>
#include "Logger.h"

namespace apl
{

AsyncPhysLayerMonitor::AsyncPhysLayerMonitor(Logger* apLogger, IPhysicalLayerAsync* apPhys, ITimerSource* apTimerSrc, millis_t aOpenRetry) :
	Loggable(apLogger),
	IHandlerAsync(apLogger),
	mpPhys(apPhys),
	mpTimerSrc(apTimerSrc),
	mpOpenTimer(NULL),
	mPortState(apLogger, "port_state"),
	mState(PLS_STOPPED),
	mIsStopping(false),
	M_OPEN_RETRY(aOpenRetry)
{
	assert(apPhys != NULL);
	assert(apTimerSrc != NULL);
	mpPhys->SetHandler(this);
}

AsyncPhysLayerMonitor::~AsyncPhysLayerMonitor()
{}

void AsyncPhysLayerMonitor::AddMonitor(IPhysMonitor* apMonitor)
{
	assert(apMonitor != NULL);
	mMonitors.insert(apMonitor);
}

void AsyncPhysLayerMonitor::WaitForStopped()
{
	this->WaitForState(PLS_STOPPED);
}

bool AsyncPhysLayerMonitor::IsStopping()
{
	return mIsStopping;
}

void AsyncPhysLayerMonitor::ChangeState(PhysLayerState aState)
{
	if(mState != aState) {
		CriticalSection cs(&mLock); // signal to anyone waiting for a state change
		mState = aState;
		LOG_BLOCK(LEV_INFO, "Transition to state: " << ConvertPhysLayerStateToString(aState));
		mPortState.Set(aState);
		for(MonitorSet::iterator i = mMonitors.begin(); i != mMonitors.end(); ++i) (*i)->OnStateChange(aState);		
		this->OnStateChange(aState);		
		cs.Broadcast();
	}
}

void AsyncPhysLayerMonitor::WaitForState(PhysLayerState aState)
{
	CriticalSection cs(&mLock);	
	while(this->GetState() != aState) cs.Wait();	
}

void AsyncPhysLayerMonitor::Start()
{
	LOG_BLOCK(LEV_INFO, "Start");
	mIsStopping = false;
	if(mpPhys->CanOpen()) {
		mpPhys->AsyncOpen();
		this->ChangeState(PLS_OPENING);
	}
}

void AsyncPhysLayerMonitor::OnOpenTimerExpiration()
{
	assert(mpOpenTimer != NULL);
	mpOpenTimer = NULL;
	this->Start();
}

void AsyncPhysLayerMonitor::Stop()
{
	LOG_BLOCK(LEV_INFO, "Stopping phys monitor");
	mIsStopping = true;
	if(mpOpenTimer) {
		mpOpenTimer->Cancel();
		mpOpenTimer = NULL;
	}
	if(mpPhys->CanClose()) {
		mpPhys->AsyncClose();
	}
	if(mpPhys->IsClosed()) this->ChangeState(PLS_STOPPED);

}

void AsyncPhysLayerMonitor::Reconnect()
{
	if(mpPhys->CanClose()) mpPhys->AsyncClose();
}

void AsyncPhysLayerMonitor::_OnOpenFailure()
{
	OnPhysicalLayerOpenFailure();
	if(mIsStopping) {
		this->ChangeState(PLS_STOPPED);
	}
	else {
		this->ChangeState(PLS_WAITING);
		mpOpenTimer = mpTimerSrc->Start(M_OPEN_RETRY, boost::bind(&AsyncPhysLayerMonitor::OnOpenTimerExpiration, this));
	}
}

void AsyncPhysLayerMonitor::_OnLowerLayerUp()
{
	this->OnPhysicalLayerOpen();
	this->ChangeState(PLS_OPEN);
}

void AsyncPhysLayerMonitor::_OnLowerLayerDown()
{
	this->OnPhysicalLayerClose();
	this->ChangeState(PLS_CLOSED);

	if(mIsStopping) this->ChangeState(PLS_STOPPED);
	else this->Start();
}

}
