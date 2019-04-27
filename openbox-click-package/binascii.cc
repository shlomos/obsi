#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <click/config.h>

CLICK_DECLS
extern const char binascii_hex_dict[] = "0123456789abcdef";

static int a2v(char c)
{
	if ((c >= '0') && (c <= '9')) {
		return c - '0';
	}
	if ((c >= 'a') && (c <= 'f')) {
		return c - 'a' + 10;
	} else {
		return 0;
	}
}

static char v2a(int c)
{
	return binascii_hex_dict[c];
}

char *unhexlify(const char *hstr)
{
	char *bstr = (char *)malloc(strlen(hstr) / 2);
	char *pbstr = bstr;
	unsigned int i = 0;
	char c;

	for (i = 0; i < strlen(hstr); i += 2)
	{
		c = (a2v(hstr[i]) << 4) + a2v(hstr[i + 1]);
		if (c == 0) {
			*pbstr++ = -128;
		} else {
			*pbstr++ = c;
		}
	}
	return bstr;
}

void unhexlify(const char *src, char *dst, size_t len)
{
	unsigned int i = 0;
	char c;

	for (i = 0; i < len; i += 2)
	{
		c = (a2v(src[i]) << 4) + a2v(src[i + 1]);
		if (c == 0) {
			*dst++ = -128;
		} else {
			*dst++ = c;
		}
	}
}

char *hexlify(const char *bstr)
{
	char *hstr = (char *)malloc(strlen(bstr) * 2);
	char *phstr = hstr;
	unsigned int i = 0;

	for (i = 0; i < strlen(bstr); i++)
	{
		if (bstr[i] == -128)
		{
			*phstr++ = '0';
			*phstr++ = '0';
		} else {
			*phstr++ = v2a((bstr[i] >> 4) & 0x0F);
			*phstr++ = v2a((bstr[i]) & 0x0F);
		}   
	}
	return hstr;
}

void hexlify(const char *src, char *dst, size_t len)
{
	unsigned int i = 0;

	for (i = 0; i < len; i++)
	{
		if (src[i] == -128)
		{
			*dst++ = '0';
			*dst++ = '0';
		} else {
			*dst++ = v2a((src[i] >> 4) & 0x0F);
			*dst++ = v2a((src[i]) & 0x0F);
		}   
	}
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(userlevel)
ELEMENT_PROVIDES(Binascii)