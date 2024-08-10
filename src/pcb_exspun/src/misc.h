#ifndef MISC_HEADER
#define MISC_HEADER


#define SETREG_LH(reg, n)\
  reg##H = n >> 8;\
  reg##L = n & 0xff

#define GETREG_LH(reg)\
  (((uint16_t)reg##H << 8) | reg##L)


#define MIN(n1, n2) (n1 < n2? n1: n2)

void _dmpfunc_pv();


#endif