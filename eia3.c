#include <stdlib.h>
#include <stdio.h>
#include "sec_type.h"
#include "ZUC.h"

//#define EIA3_PRINT
//#define EIA3_PRINT_2

static u32 GET_WORD(u32 * DATA, u32 i)
{
	u32 WORD, ti;
	ti = i % 32;
	if (ti == 0) {
		WORD = DATA[i/32];
	}
	else {
		WORD = (DATA[i/32]<<ti) | (DATA[i/32+1]>>(32-ti));
	}
	return WORD;
}

static u8 GET_BIT(u8 * DATA, u32 i)
{
	return (DATA[i/8] & (1<<(7-(i%8)))) ? 1 : 0;
}

void eia3(UINT8 *key, INT32 count, INT32 bearer, INT32 dir, UINT8 *data, INT32 length, UINT8 *outMac)
{
	u32 *KS;
	u32 L = (length+31) / 32 + 2; 
	u32 T = 0, retMac = 0;
	u32 i;
	u8 IV[16];
	
	KS      = (u32 *) malloc(L*sizeof(u32));
	IV[0]  = (count>>24) & 0xFF;
	IV[1]  = (count>>16) & 0xFF;
	IV[2]  = (count>>8) & 0xFF;
	IV[3]  = count & 0xFF;
	IV[4]  = (bearer << 3) & 0xF8;
	IV[5]  = 0;
	IV[6]  = 0;
	IV[7]  = 0;
	IV[8]  = IV[0] ^ ((dir&1) << 7);
	IV[9]  = IV[1];
	IV[10] = IV[2];
	IV[11] = IV[3];
	IV[12] = IV[4];
	IV[13] = IV[5];
	IV[14] = IV[6] ^((dir&1) << 7);
	IV[15] = IV[7];

	#ifdef EIA3_PRINT
		printf("EIA3 Key:\n");
		for (i = 0;i < 16;i++)
		{
			printf("%02X ", key[i]);
		}
		printf("\n");
	#endif

#ifdef EIA3_PRINT
		printf("EIA3 IV:\n");
		for (i = 0;i < 16;i++)
		{
			printf("%02X", IV[i]);
		}
		printf("\n");
#endif	

	ZUC(key,IV,KS,L);

#ifdef EIA3_PRINT
	printf("EIA3 KS:\n");
	for (i = 0; i < L;i++)
	{
		printf("%08X\n", KS[i]);
	}
#endif

	for (i = 0; i < length; i++)
	{	
		if (GET_BIT(data,i)) {
#ifdef EIA3_PRINT_2
			printf("T_0x%08x ^ GET_WORD(KS,%d)_0x%08x = 0x%08x\n", T, i, GET_WORD(KS,i), T^GET_WORD(KS,i));
#endif
			T ^= GET_WORD(KS,i);			
		}

	}
		
	T ^= GET_WORD(KS, length);

	
	retMac = T ^ KS[L-1];

	for(i=0; i<4; i++)
	{
		outMac[i] = (u8)(retMac >> (3-i)*8 );
	}


#ifdef EIA3_PRINT_2
	printf("\n");
#endif

    free(KS);
}

