/*
 * StateMachineGenerator.h
 *
 *  Created on: Jan 12, 2011
 *      Author: yotamhc
 */

#ifndef STATEMACHINEGENERATOR_H_
#define STATEMACHINEGENERATOR_H_

#include "StateMachine.h"
#include "../AhoCorasick/ACBuilder.h"

CLICK_DECLS
#ifdef	__cplusplus
extern "C" {
#endif

StateMachine *createStateMachine(ACTree *tree, int maxGotosLE, int maxGotosBM, int verbose);
StateMachine *createSimpleStateMachine(const char *path);
void destroyStateMachine(StateMachine *machine);

#ifdef	__cplusplus
}
#endif
CLICK_ENDDECLS

#endif /* STATEMACHINEGENERATOR_H_ */
