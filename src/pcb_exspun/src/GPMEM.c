#include "stm8s.h"


bool _gpmem_isused = FALSE;
char _gpmem[124];

char *useGPMEM(){
  if(!_gpmem_isused){
    _gpmem_isused = TRUE;
    return _gpmem;
  }

  return 0;
}

void releaseGPMEM(){
  _gpmem_isused = FALSE;
}