/*
 * TableStateMachineGenerator.h
 *
 *  Created on: Jan 25, 2011
 *      Author: yotamhc
 */

#ifndef TABLESTATEMACHINEGENERATOR_H_
#define TABLESTATEMACHINEGENERATOR_H_

#include "TableStateMachine.h"

CLICK_DECLS
#ifdef	__cplusplus
extern "C" {
#endif

TableStateMachine *generateTableStateMachine(const char *path, int num_commons, double uncommon_rate_limit, const char *common_marks_path, const char *common_reorder_map_path, int verbose);

#ifdef	__cplusplus
}
#endif
CLICK_ENDDECLS

#endif /* TABLESTATEMACHINEGENERATOR_H_ */
