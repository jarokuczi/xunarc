//
// Created by jarokuczi on 26/08/2021.
//

#include "common.h"
#include <inline/dos.h>

#ifdef LOG_ENABLED
BPTR output;
#endif

void Log(char * str) {
#ifdef LOG_ENABLED
    FPrintf(output, str);
#endif
}