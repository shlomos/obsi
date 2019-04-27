/*
 * ACBuilder.h
 *
 *  Created on: Jan 12, 2011
 *      Author: yotamhc
 */

#ifndef ACBUILDER_H_
#define ACBUILDER_H_
#include "ACTypes.h"
#include "../Common/Flags.h"

CLICK_DECLS
#ifdef	__cplusplus
extern "C" {
#endif


void acBuildTree(ACTree *tree, const char *path, int avoidFailToLeaves, int mixIDs);
void acDestroyTreeNodes(ACTree *tree);
Node *acGetNextNode(Node *node, unsigned char c);
void acPrintTree(ACTree *tree);
int acAddPatternToTree(ACTree *tree, unsigned char *pattern, unsigned int length);
void acFinalize(ACTree *tree, int avoidFailToLeaves, int mixID);
void acReorderStates(ACTree *tree, int *old2new);
#ifdef FIND_MC2_BAD_PATH
int acFindBadMC2Path(ACTree *tree, int *path_states, char *path_chars, int max_size);
int acCountUncommonStates(int *path_states, int size);
#endif

#ifdef	__cplusplus
}
#endif
CLICK_ENDDECLS
#endif /* ACBUILDER_H_ */
