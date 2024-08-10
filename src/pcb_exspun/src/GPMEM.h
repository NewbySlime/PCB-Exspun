#ifndef GPMEM_HEADER
#define GPMEM_HEADER

#include "string.h"

#ifndef GPMEM_BUFSIZE
#error Please define GPMEM buffer size as GPMEM_BUFSIZE
#endif

// General Purpose Memory

// return NULL if still in use
char *useGPMEM();
void releaseGPMEM();

#endif