/*
 * module.h
 *
 *  Created on: Mar 22, 2010
 *      Author: henk
 */

#ifndef MODULE_H_
#define MODULE_H_

#include "redis.h"

extern Module *g_module;

void Module_dispatch();

#endif /* MODULE_H_ */
