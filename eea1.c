#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sec_type.h"
#include "SNOW_3G.h"

//#define EEA1_PRINT
//#define EEA1_PRINT_2

/* eea1.
* Input key: 128 bit Confidentiality Key.
* Input count:32-bit Count, Frame dependent input.
* Input bearer: 5-bit Bearer identity (in the LSB side).
* Input dir:1 bit, direction of transmission.
* Input data: length number of bits, input bit stream.
* Input length: 16 bit Length, i.e., the number of bits to be encrypted or
* decrypted.
* Output data: Output bit stream. Assumes data is suitably memory
* allocated.
* Encrypts/decrypts blocks of data between 1 and 20000 bits in length as
* defined in Section 3.
*/
void eea1( UINT8 *key, INT32 count, INT32 bearer, INT32 dir, UINT8 *data, INT32 length, UINT32 offset, UINT8 *dataOut)
{
	u32 K[4],IV[4];
	int n = (length + offset  + 31 ) / 32, sum = 0;
	int endRes = ((length + offset) % 32) / 8;  //assume that the data is byte-aligned. 
	int startPos = offset / 32, startRes = (offset % 32) / 8;
	int h = 0, k = 0;
	int i=0, j=0, m = 0;
	u32 *KS;
	/*Initialisation*/
	/* Load the confidentiality key for SNOW 3G initialization as in section
	3.4. */
	K[3] = (key[0] << 24) + (key[1] << 16) + (key[2] << 8) + key[3];
	K[2] = (key[4] << 24) + (key[5] << 16) + (key[6] << 8) + key[7];
	K[1] = (key[8] << 24) + (key[9] << 16) + (key[10] << 8) + key[11];
	K[0] = (key[12] << 24) + (key[13] << 16) + (key[14] << 8) + key[15];

	/* Prepare the initialization vector (IV) for SNOW 3G initialization as insection 3.4. */
	IV[3] = count;
	IV[2] = (bearer << 27) | ((dir & 0x1) << 26);
	IV[1] = IV[3];
	IV[0] = IV[2];

	//eea1_print add by zhangyuanliang
	#ifdef EEA1_PRINT
		for (i=0;i<4;i++)
		{
			printf("K%d = %0x\n", i, K[i]);
			printf("IV%d = %0x\n", i, IV[i]);
		}
	#endif
	/* Run SNOW 3G algorithm to generate sequence of key stream bits KS*/
	Initialize(K,IV);

	KS = (u32 *)malloc(4*n);
	GenerateKeystream(n,(u32*)KS);
	/* Exclusive-OR the input data with keystream to generate the output bit stream */
	#ifdef EEA1_PRINT_2
		printf("EEA1 KS:\n");
	#endif
	for (i=startPos; i<n; i++)
	{
#ifdef EEA1_PRINT_2
	    printf("cipher block[%d] = ", i);
#endif
	    h = (i == startPos) ? startRes : 0;
	    k = (i == n - 1) ? ((endRes == 0) ? 4 : endRes) : 4;

	    for (j=h; j<k; j++, m++)
	    {
#ifdef EEA1_PRINT_2
	//	printf("%02X", (u8)(*(KS+i)>>((3-j)*8)));
#endif
		dataOut[m] = data[m] ^ (u8)(*(KS+i)>>((3-j)*8));
#ifdef EEA1_PRINT_2
		printf("%02X", dataOut[m]);
#endif

		sum += 8;
		if (sum >= length)
		{
		    break;
		}
	    }
#ifdef EEA1_PRINT_2
	    printf("\n");
#endif
	}

	free(KS);
}
/* End of f8.c */
