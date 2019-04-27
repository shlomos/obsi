#ifndef __BINASCII_H_
#define __BINASCII_H_

#include <click/batchelement.hh>

CLICK_DECLS

char *unhexlify(const char *hstr);
char *hexlify(const char *bstr);

CLICK_ENDDECLS

#endif