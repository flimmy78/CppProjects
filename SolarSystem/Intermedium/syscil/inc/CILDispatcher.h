/*
 * CILDispatcher.h
 *
 *  Created on: Jun 13, 2015
 *      Author: dinglinhui
 */

#ifndef CILDISPATCHER_H_
#define CILDISPATCHER_H_

#include "OSThreadEx.h"

namespace syscil {
using namespace osext;

class CILDispatcher: public OSThreadEx {
public:
	CILDispatcher();
	virtual ~CILDispatcher();

private:
	virtual OSRet Run() override final;
};

} /* namespace syscil */

#endif /* CILDISPATCHER_H_ */