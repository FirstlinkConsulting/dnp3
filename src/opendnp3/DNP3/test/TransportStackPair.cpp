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

#include "TransportStackPair.h"

#include <boost/asio.hpp>

#include <opendnp3/APL/Logger.h>

namespace apl
{
namespace dnp
{

TransportStackPair::TransportStackPair(
        LinkConfig aClientCfg,
        LinkConfig aServerCfg,
        Logger* apLogger,
        boost::asio::io_service* apService,
        ITimerSource* apTimerSrc,
        boost::uint16_t aPort) :

	mClient(apLogger->GetSubLogger("TCPClient"), apService, "127.0.0.1", aPort),
	mServer(apLogger->GetSubLogger("TCPServer"), apService, "127.0.0.1", aPort),
	mClientStack(apLogger->GetSubLogger("ClientStack"), apTimerSrc, &mClient, aClientCfg),
	mServerStack(apLogger->GetSubLogger("ServerStack"), apTimerSrc, &mServer, aServerCfg)
{

}

bool TransportStackPair::BothLayersUp()
{
	return mServerStack.mUpper.IsLowerLayerUp()
	       && mClientStack.mUpper.IsLowerLayerUp();
}

void TransportStackPair::Start()
{
	mServerStack.mRouter.Start();
	mClientStack.mRouter.Start();
}


}
}

