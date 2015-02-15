#ifndef _LFSR113_C_
#define _LFSR113_C_
#include "lfsr113.h"

/*
   32-bits Random number generator U[0,1): lfsr113
   Author: Pierre L'Ecuyer,
   Source: http://www.iro.umontreal.ca/~lecuyer/myftp/papers/tausme2.ps
   ---------------------------------------------------------
*/

static unsigned int z1;
static unsigned int z2;
static unsigned int z3;
static unsigned int z4;

unsigned int lfsr113(void)
{
   unsigned int b;
   b  = ((z1 << 6) ^ z1) >> 13;
   z1 = ((z1 & 4294967294U) << 18) ^ b; 
   b  = ((z2 << 2) ^ z2) >> 27; 
   z2 = ((z2 & 4294967288U) << 2) ^ b;
   b  = ((z3 << 13) ^ z3) >> 21;
   z3 = ((z3 & 4294967280U) << 7) ^ b;
   b  = ((z4 << 3) ^ z4) >> 12;
   z4 = ((z4 & 4294967168U) << 13) ^ b;
   return (z1 ^ z2 ^ z3 ^ z4);
}

// This should be called ONCE before using lfsr113
void srand_lfsr113(unsigned int seed) {
  z1 = z2 = z3 = z4 = seed;
}

#endif // _LFSR113_C_ 
