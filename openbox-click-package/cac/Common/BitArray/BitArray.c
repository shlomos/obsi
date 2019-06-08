/*
 * BitArray.c
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */

#include "BitArray.h"
#include "../../AhoCorasick/ACTypes.h"

CLICK_DECLS

#ifdef COUNT_CALLS
static int counter = 0;
#endif

#ifdef COUNT_CALLS
int getCounter_BitArray() {
	return counter;
}
#endif

CLICK_ENDDECLS
ELEMENT_PROVIDES(BitArrayC)
