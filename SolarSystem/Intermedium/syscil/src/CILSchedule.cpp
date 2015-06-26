/*
 * CILSchedule.cpp
 *
 *  Created on: Jun 13, 2015
 *      Author: dinglinhui
 */

#include "CILSchedule.h"

namespace syscil {
CILSchedule::CILSchedule() {
}

CILSchedule::~CILSchedule() {
}

OSRet CILSchedule::Run() {
	OSHeartbeat &heartbeat = this->GetHeartbeat();
	while (true) {
		try {
			heartbeat++;
			std::cout << "[CILSchedule]" << heartbeat << std::endl << std::flush;
			this->SetThreadStatus(TStat::Running);

			std::this_thread::sleep_for(std::chrono::milliseconds(OS_THREAD_PAUSE));

		} catch (std::exception const& ex) {
			std::cerr << "Exception: " << ex.what() << std::endl;
		}
	}
	return OSRet::OK;
}

} /* namespace syscil */
