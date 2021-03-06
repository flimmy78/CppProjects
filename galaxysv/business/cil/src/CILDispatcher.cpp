/*
 * CILDispatcher.cpp
 *
 *  Created on: Jun 13, 2015
 *      Author: dinglinhui
 */

#include "../../cil/inc/CILDispatcher.h"

namespace syscil {

CILDispatcher::CILDispatcher(int nPrio, int nStackSize, int nQueueSize) :
		OSThreadEx(nPrio, nStackSize, nQueueSize) {
}

CILDispatcher::~CILDispatcher() {
}

OSRet CILDispatcher::run() {
	OSHeartbeat &heartbeat = this->getHeartbeat();
	while (true) {
		try {
			heartbeat++;
			std::cout << "[CILDispatcher]" << heartbeat << std::endl << std::flush;
			this->setThreadStatus(TStat::Running);

			std::this_thread::sleep_for(std::chrono::milliseconds(OS_THREAD_PAUSE));

		} catch (std::exception const& ex) {
			std::cerr << "Exception: " << ex.what() << std::endl;
		}
	}
	return OSRet::OK;
}

} /* namespace syscil */
