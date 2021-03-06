/*
 * CILService.h
 *
 *  Created on: Jun 13, 2015
 *      Author: dinglinhui
 */

#ifndef CILSERVICE_H_
#define CILSERVICE_H_

#include "../../cil/inc/CILDevice.h"
#include "OSThreadEx.h"
#include "OSThreadPool.h"
#include "OSMessageBase.h"

namespace syscil {
using namespace osext;

class CILDevice;
class OSThreadPool;
class CILService: public OSThreadEx {
public:
	CILService(int nPrio, int nStackSize, int nQueueSize);
	virtual ~CILService();

	void Install(CILDevice *pDevice);
	void UnInstall(CILDevice *pDevice);
	void SetSchedPool(OSThreadPool *pPool);
	void SetPackEntry(OSMessageBase *ptr);

	CILDevice* FindDevice(int nIdent);
	OSThreadPool* GetSchedPool(void) const;
	OSMessageBase* GetPackEntry(void) const;

protected:
	virtual OSRet run() override final;
	virtual OSRet OSInitHook(void) override;
	virtual int ReceiveMessage(OSMessage *msg) override;
	virtual int OnHandleMessage(OSMessage *msg) override;

private:
	CILDevice* m_plstDevice;
	OSMessageBase* m_pEntry;
	OSThreadPool* m_pShedPool;
};

} /* namespace syscil */

#endif /* CILSERVICE_H_ */
