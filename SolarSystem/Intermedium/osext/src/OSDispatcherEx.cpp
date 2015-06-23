/*
 * OSDispatcherEx.cpp
 *
 *  Created on: Jun 13, 2015
 *      Author: dinglinhui
 */

#include "OSDispatcherEx.h"
#include <cassert>
#include <cstdlib>
#include <stdexcept>

namespace osext {
utils::MemPool<OSMessage> OSDispatcherEx::mempool(OS_MAX_MESSAGE_NUM); //预先分配10个OSMessage的内存空间

OSDispatcherEx* OSDispatcherEx::m_pDispatcher = nullptr;

////////////////////////////////////////////////////////////////////////////////
//
// global function
//
////////////////////////////////////////////////////////////////////////////////
OSDispatcherEx* GetOSDispatcher(void) {
	return OSDispatcherEx::m_pDispatcher;
}

int OSSendMessage(int nPrio, MSGType nCmd, DWORD wParam, DWORD lParam) {
	OSDispatcherEx *pDispatcher = GetOSDispatcher();
	if (pDispatcher == nullptr)
		return 1;

//	BYTE prio = (BYTE) ((nCmd & 0xFF000000) >> 24);
//	if (prio != 0)
//		return 2;

	if (nCmd == MSGType::MSG_NULL) {
		return pDispatcher->SendMessage((OSMessageBase*) pDispatcher, nCmd,
				wParam, lParam);
	} else {
		OSThreadEx *pThread = pDispatcher->Find(std::this_thread::get_id());
		if (pThread != nullptr) {
			return pDispatcher->SendMessage((OSMessageBase*) pThread, nCmd,
					wParam, lParam);
		}
	}
	return 3;
}

int OSPostMessage(int nPrio, MSGType nCmd, DWORD wParam, DWORD lParam) {
	OSDispatcherEx *pDispatcher = GetOSDispatcher();
	if (pDispatcher != nullptr) {
//		DWORD nNewCmd = (nPrio << 24) | nCmd;
		MSGType nNewCmd = MSGType::MSG_ACT;
		return pDispatcher->PostMessage(pDispatcher, nNewCmd, wParam, lParam);
	}
	return -1;
}

void *OSGetMessagePtr(void) {
	OSDispatcherEx *pDispatcher = GetOSDispatcher();
	if (pDispatcher != nullptr)
		return pDispatcher->GetMessagePtr();
	return nullptr;
}

void OSPutMessagePtr(void *p) {
	OSDispatcherEx *pDispatcher = GetOSDispatcher();
	if (pDispatcher != nullptr)
		pDispatcher->PutMessagePtr(p);
}

////////////////////////////////////////////////////////////////////////////////
//
// class OSDispatcherEx
//
////////////////////////////////////////////////////////////////////////////////
OSDispatcherEx::OSDispatcherEx() {
	m_pThreadList = nullptr;
}

OSDispatcherEx::~OSDispatcherEx() {
}

OSRet OSDispatcherEx::Start(void) {
	if (!QueueInitialize())
		return OSRet::ERROR;

	m_pDispatcher = this;
	return OSDispatcher::Start();
}

OSRet OSDispatcherEx::Stop(void) {
	OSThreadEx *temp, *task = m_pThreadList;

	while (task != nullptr) {
		temp = task->m_pNext;
		UnRegister(task);
		task = temp;
	}

	m_pDispatcher = nullptr;
	return OSRet::OK;
}

int OSDispatcherEx::Register(OSThreadEx* pService) {
	OSThreadEx **ppPrev = &m_pThreadList;
	while (*ppPrev != nullptr) {
		ppPrev = &(*ppPrev)->m_pNext;
	}

	pService->m_pNext = *ppPrev;
	*ppPrev = pService;

	return 0;
}

int OSDispatcherEx::UnRegister(OSThreadEx* pService) {
	OSThreadEx **ppPrev = &m_pThreadList;

	while (*ppPrev != nullptr && *ppPrev != pService) {
		ppPrev = &(*ppPrev)->m_pNext;
	}

	if (*ppPrev != nullptr) {
		*ppPrev = pService->m_pNext;
	}
	return 0;
}

OSThreadEx* OSDispatcherEx::Find(std::thread::id nID) const {
	OSThreadEx* pTop = m_pThreadList;

	while (pTop != nullptr) {
		if (nID == (pTop)->GetThreadID())
			return (pTop);
		pTop = (pTop)->m_pNext;
	}
	return nullptr;
}

void OSDispatcherEx::SendMessageToDescendants(OSMessage* msg) {
	if (msg != nullptr) {
		SendMessageToDescendants(msg->m_nCmd, msg->m_wParam, msg->m_lParam);
		delete msg;
	}
}

void OSDispatcherEx::SendMessageToDescendants(MSGType nCmd, DWORD wParam,
		DWORD lParam) {
	OSThreadEx* pTop = m_pThreadList;
	while (pTop != nullptr) {
		SendMessage(pTop, nCmd, wParam, lParam);
		pTop = pTop->m_pNext;
	}
}

void OSDispatcherEx::PostMessageToDescendants(OSMessage* msg) {
	if (msg != nullptr) {
		PostMessageToDescendants(msg->m_nCmd, msg->m_wParam, msg->m_lParam);
		delete msg;
	}
}

void OSDispatcherEx::PostMessageToDescendants(MSGType nCmd, DWORD wParam,
		DWORD lParam) {
	OSThreadEx* pTop = m_pThreadList;
	while (pTop != nullptr) {
		PostMessage(pTop, nCmd, wParam, lParam);
		pTop = pTop->m_pNext;
	}
}

void* OSDispatcherEx::GetMessagePtr(void) {
	return mempool.alloc();
}

void OSDispatcherEx::PutMessagePtr(void* p) {
	mempool.dealloc((OSMessage*) p);
}

OSRet OSDispatcherEx::Run(void) {
	//Notify all thread FM_CREATE message
	PostMessage((OSMessageBase *) this, MSGType::MSG_CREATE, 0, 0);
	//
	InitThreadList();

	while (true) {
		try {
			OSMessage *msg = nullptr;
			if (Pend(msg) == 0) {
				if (msg != nullptr) {
					OnHandleMessage(msg);
					delete msg;
				}
			}
			event(1, 1);
		} catch (std::exception const& ex) {
			std::cerr << "Exception: " << ex.what() << std::endl;
		}
	}
	return OSRet::OK;
}

OSRet OSDispatcherEx::OSInitHook(void) {
	OSRet nRet = OSRet::OK;

	OSThreadEx* pTop = m_pThreadList;
	while (pTop != NULL) {
		nRet = (pTop)->Start();
		if (nRet != OSRet::OK)
			return nRet;
		pTop = pTop->m_pNext;
	}

	return OSDispatcher::OSInitHook();
}

int OSDispatcherEx::OnHandleMessage(OSMessage* msg) {
	int nRet = -1;

	if (msg != nullptr) {
		switch (msg->m_nCmd) {
		case MSGType::MSG_GETSERVICE: {
			OSThreadEx* pService = Find(std::this_thread::get_id());
			if (msg->m_bAsyn) {
				PostMessage((OSMessageBase*) msg->m_pSource, msg->m_nCmd,
						msg->m_wParam, (DWORD) pService, nullptr);
			} else {
				msg->m_lParam = (DWORD) pService;
			}
		}
			break;

		default:
			nRet = OSMessageBase::OnHandleMessage(msg);
			break;
		}
		return nRet;
	}
	return -1;
}

int OSDispatcherEx::ReceiveMessage(OSMessage* msg) {
	if (msg == nullptr)
		return 1;

	BYTE nPrio = 0;
//	BYTE nPrio = (BYTE) ((msg->m_nCmd & 0xFF000000) >> 24);
//	msg->m_nCmd &= 0x00FFFFFF;

	int nRet = 2;
	if (nPrio == 0) {
		nRet = OSMessageBase::ReceiveMessage(msg);
	} else {
		if (nPrio == 0xFF) {
			PostMessageToDescendants(msg);
			nRet = 0;
		} else {
			OSThreadEx *pThread = Find(std::this_thread::get_id());
			if (pThread != nullptr) {
				nRet = PostMessage((OSMessageBase*) pThread, msg);
			}
		}
	}

	return nRet;
}

void OSDispatcherEx::InitThreadList(void) {
	OSThreadEx* pTop = m_pThreadList;
	while (pTop != nullptr) {
		event += [&pTop](ULONGLONG pre, ULONGLONG cur) {
			OSHeartbeat &heartbeat = pTop->GetHeartbeat();
			heartbeat.print();
			if(heartbeat.isEqual()) {
				throw std::logic_error("thread heartbeat error");
			} else {
				heartbeat--;
			}
		};

		pTop = (pTop)->m_pNext;
	}

}
}