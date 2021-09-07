//
// Created by jarokuczi on 26/08/2021.
//

#ifndef XUNARC_COMMON_H
#define XUNARC_COMMON_H

#include <exec/types.h>
#include <inline/dos.h>
#include <sys/types.h>

struct DosLibrary *	 DOSBase;
struct xadMasterBase *	xadMasterBase;

struct Config {
    STRPTR src;
    STRPTR dst;
    STRPTR pattern;
    BPTR output;
    short outputClosable;
    //no absolute paths - may be usefull in the future - by default disabled
    ULONG noabs;
    uint quiet;
};

void Log(char * str);
#endif //XUNARC_COMMON_H
