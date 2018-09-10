/*
 * ACTypes.h
 *
 *  Created on: Jan 12, 2011
 *      Author: yotamhc
 */

#ifndef ACTYPES_H_
#define ACTYPES_H_

#include "../Common/HashMap/HashMap.h"

#ifndef CLICK_DECLS
# define CLICK_DECLS        /* */
# define CLICK_ENDDECLS     /* */
# define CLICK_USING_DECLS  /* */
#endif
#ifndef EXPORT_ELEMENT    
# define EXPORT_ELEMENT(x)
# define ELEMENT_REQUIRES(x)
# define ELEMENT_PROVIDES(x)
# define ELEMENT_HEADER(x)
# define ELEMENT_LIBS(x)
# define ELEMENT_MT_SAFE(x)
#endif

struct st_node;

typedef struct {
	char c;
	struct st_node *ptr;
} Pair;

typedef struct st_node {
	int id;
	int numGotos;
	int match;
	char *message;
	int messageLength;
	HashMap *gotos;
	struct st_node *failure;
	int hasFailInto; // TRUE if some other node fails into this node
	int depth;
	char c1, c2;
	int isFirstLevelNode;
	int isSecondLevelNode;
	int marked;
} Node;

#ifdef PCRE
typedef struct {
	int state;
	char *pcre;
} StateRegexPair;
#endif

typedef struct {
	Node *root;
	int size;
#ifdef PCRE
	HashMap *state_pcre_map;
#endif
} ACTree;

#endif /* ACTYPES_H_ */
