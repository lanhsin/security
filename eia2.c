#include <stdio.h>
#include <stdlib.h>
#include "sec_type.h"
#include "AES_Encrypt.h"

// print for debug
//#define prtDebug 1
// test AES_CMAC
//#define tAES_CMAC 1

/* For CMAC Calculation */
unsigned char const_Rb[16] = 
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
};
unsigned char const_Zero[16] = 
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void xor_128(unsigned char *a, unsigned char *b, unsigned char *out)
{
	int i;
	for (i=0;i<16; i++)
	{
		out[i] = a[i] ^ b[i];
	}
}


#ifdef prtDebug

static void print_hex(char *str, unsigned char *buf, int len)
{
	int i;
	for ( i=0; i<len; i++ ) {
		if ( (i % 16) == 0 && i != 0 ) printf(str);
		printf("%02x", buf[i]);
		if ( (i % 4) == 3 ) printf(" ");
		if ( (i % 16) == 15 ) printf("\n");
	}
	if ( (i % 16) != 0 ) printf("\n");
}

static void print128(unsigned char *bytes)
{
	int j;
	for (j=0; j<16;j++) {
		printf("%02x",bytes[j]);
		if ( (j%4) == 3 ) printf(" ");
	}
}

#endif

/* AES-CMAC Generation Function */
static void leftshift_onebit(unsigned char *input,unsigned char *output)
{
	int i;
	unsigned char overflow = 0;
	for ( i=15; i>=0; i-- ) {
		output[i] = input[i] << 1;
		output[i] |= overflow;
		overflow = (input[i] & 0x80)?1:0;
	}
	return;
}

static void generate_subkey(unsigned char *key, unsigned char *K1, unsigned char *K2)
{
	unsigned char L[16];
	unsigned char Z[16];
	unsigned char tmp[16];
	int i;
	for ( i=0; i<16; i++ ) Z[i] = 0;
	getKS(key,Z,L);
	if ( (L[0] & 0x80) == 0 ) { /* If MSB(L) = 0, then K1 = L << 1 */
		leftshift_onebit(L,K1);
	} else { /* Else K1 = ( L << 1 ) (+) Rb */
		leftshift_onebit(L,tmp);
		xor_128(tmp,const_Rb,K1);
	}
	if ( (K1[0] & 0x80) == 0 ) {
		leftshift_onebit(K1,K2);
	} else {
		leftshift_onebit(K1,tmp);
		xor_128(tmp,const_Rb,K2);
	}
	
#ifdef prtDebug
	printf("L "); print128(L); printf("\n");
	printf("K1 "); print128(K1); printf("\n");
	printf("K2 "); print128(K2); printf("\n");
#endif
	return;
}

static void padding ( unsigned char *lastb, unsigned char *pad, int length )
{
	int j;
	/* original last block */
	for ( j=0; j<16; j++ ) {
		if ( (j+1)*8 < length) 
		{
			pad[j] = lastb[j];
		}
		else if ((j+1)*8 > length && j*8 < length)
		{
			pad[j] = lastb[j];
			pad[j] |= (u8)(1<<((j+1)*8 - length - 1));
		}
		else if ( (j+1)*8 == length )
		{
			pad[j] = lastb[j];
			j++;
			pad[j] = 0x80;
		} 
		else if( (j+1)*8 > length )
		{
			pad[j] = 0x00;
		}
	}
}

static void AES_CMAC ( unsigned char *key, unsigned char *input, int length, unsigned char *mac, int remainder128)
{
	unsigned char X[16],Y[16], M_last[16], padded[16];
	unsigned char K1[16], K2[16];
	int n, i, flag;
	generate_subkey(key,K1,K2);
	n = (length+15) / 16; /* n is number of rounds */
	if ( n == 0 ) {
		n = 1;
		flag = 0;
	} else {
		if ( (length%16) == 0 && remainder128 == 0) { /* last block is a complete block */
			flag = 1;
		} else { /* last block is not complete block */
			flag = 0;
		}
	}
	if ( flag ) { /* last block is complete block */
		xor_128(&input[16*(n-1)],K1,M_last);
	} else {
		padding(&input[16*(n-1)],padded,remainder128);
#ifdef prtDebug
		printf("padded "); print128(padded); printf("\n");
#endif
		xor_128(padded,K2,M_last);
	}
#ifdef prtDebug
	printf("M_last "); print128(M_last); printf("\n");
#endif
	for ( i=0; i<16; i++ ) X[i] = 0;
	for ( i=0; i<n-1; i++ ) {
		xor_128(X,&input[16*i],Y); /* Y := Mi (+) X */
		getKS(key,Y,X); /* X := AES-128(KEY, Y); */
#ifdef prtDebug
		printf("X%d ", i); print128(X); printf("\n");
#endif
	}
	xor_128(X,M_last,Y);
	getKS(key,Y,X);
#ifdef prtDebug
	printf("last Y "); print128(Y); printf("\n");
	printf("last X "); print128(X); printf("\n");
#endif
	for ( i=0; i<4; i++ ) {
		mac[i] = X[i];
	}
}


void eia2(UINT8 *key, INT32 count, INT32 bearer, INT32 dir, UINT8 *data, INT32 length, UINT8 *outMac)
{
	int mLen = (length+64+7)/8;
	int remainder128 = (length+64)%128;
	int loopi = 0, loopj = 0;

	u8 *M;
	M = (u8*)malloc(mLen);
	/* from count & bearer & dir & data generate M*/
	for(loopi=0;loopi<mLen;loopi++) 
	{
		if (loopi<4)//count
		{
			M[loopi] = (u8)(count>>(8*(3-loopi)));
		}
		else if (loopi == 4)//bearer+dir
		{
			M[loopi] = ((u8)bearer<<3) | ((u8)dir<<2);
		}
		else if(loopi < 8)//0x00
		{
			M[loopi] = 0;
		}
		else
		{
			M[loopi] = data[loopj];
			loopj++;
		}
	}

#ifdef prtDebug
	printf("--------------------------------------------------\n");
	printf("K "); print128(key); printf("\n");
	printf("\nSubkey Generation\n");

#ifdef tAES_CMAC
	printf("\nExample : len = %d\n", 64);
	printf("M "); print_hex(" ",M,64);
#else
	printf("\nExample : len = %d\n", mLen);
	printf("M "); print_hex(" ",M,mLen);
#endif
	
#endif

	AES_CMAC(key,M,mLen,outMac,remainder128);
	
#ifdef prtDebug
	printf("AES_CMAC "); 
	print128(outMac); 
	printf("\n");
	printf("--------------------------------------------------\n");
#endif

	free(M);
}

