/*
 * BitArray.h
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */

#ifndef BITARRAY_H_
#define BITARRAY_H_

#include "../Types.h"

#define   MASK_BITS_0	0x001
#define  MASK_BITS_01	0x003
#define MASK_BITS_012	0x007
#define MASK_BITS_123	0x00E
#define MASK_BITS_234	0x01C
#define MASK_BITS_345	0x038
#define MASK_BITS_456	0x070
#define MASK_BITS_567	0x0E0
#define MASK_BITS_67	0x0C0
#define MASK_BITS_7		0x080

#define GET_MATCH_BIT(value) ((value) & 0x01)
#define GET_PTR_TYPE(value) (((value) >> 1) & 0x03)

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
/*
#define GET_3BITS_ELEMENT(arr, i) \
		(((((3 * (i)) % 8) < 6) ? ((arr)[(3 * (i)) / 8] & MASK_BITS[(3 * (i)) % 8]) : 	\
				(((3 * (i)) % 8) == 6) ?											\
						(((arr)[(3 * (i)) / 8 + 1] & MASK_BITS_0) << 8) |							\
						 ((arr)[(3 * (i)) / 8] & MASK_BITS_67) :								\
							 (((3 * (i)) % 8) == 7) ?								\
										(((arr)[(3 * (i)) / 8 + 1] & MASK_BITS_01) << 8) |		\
										 ((arr)[(3 * (i)) / 8] & MASK_BITS_7) : 0) >> ((3 * (i)) % 8))
*/

#ifdef COUNT_CALLS
int getCounter_BitArray();
#endif

static const uchar MASK_BITS[] = {
		MASK_BITS_012,
		MASK_BITS_123,
		MASK_BITS_234,
		MASK_BITS_345,
		MASK_BITS_456,
		MASK_BITS_567
};

static const unsigned char POW2[] = { 0, 2, 4, 8, 16, 32, 64, 128 };

inline uchar GET_3BITS_ELEMENT(uchar *arr, int i) {
	int bit = (3 * i) % 8;
	int byte = (3 * (i)) / 8;

	return (((arr[byte] >> bit) % (POW2[min(8 - bit, 3)]))) |
			((bit > 5) ? (((arr[byte + 1]) % (POW2[bit - 5])) << (max(8 - bit, 0))) : 0);
}

inline void SET_3BITS_ELEMENT(uchar *arr, int i, uchar value) {
	int bit = (3 * (i)) % 8;
	int byte = (3 * (i)) / 8;

	if (bit < 6) {
		arr[byte] = arr[byte] | (((uchar)(value << bit)) & MASK_BITS[bit]);
	} else if (bit == 6) {
		arr[byte] = arr[byte] | (((uchar)(value << 6)) & MASK_BITS_67);
		arr[byte + 1] = arr[byte + 1] | (((uchar)((value << 6) >> 8)) & MASK_BITS_0);
	} else if (bit == 7) {
		arr[byte] = arr[byte] | (((uchar)(value << 7)) & MASK_BITS_7);
		arr[byte + 1] = arr[byte + 1] | (((uchar)((value << 7) >> 8)) & MASK_BITS_01);
	}
}


#define GET_1BIT_ELEMENT(arr, i) (((arr)[(i) / 8] >> ((i) % 8)) & 0x01)

#define SET_1BIT_ELEMENT(arr, i, value) \
	(arr)[(i) / 8] = ((((arr)[(i) / 8]) & 0xFF) | (((value) & 0x01) << ((i) % 8)))

#define GET_2BITS_ELEMENT(arr, i) (((arr)[((i) / 4)] >> (2 * ((i) % 4))) & 0x03)

#define SET_2BITS_ELEMENT(arr, i, value) \
	(arr)[(i) / 4] = ((((arr)[(i) / 4]) & 0xFF) | (((value) & 0x03) << (2 * ((i) % 4))))

#endif //BITARRAY_H_
